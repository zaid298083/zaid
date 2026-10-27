#include "nexion_minimap.h"

#include "../main.h"
#include "../java/jniutil.h"
#include "../game/game.h"
#include "../game/playerped.h"
#include "../game/vehicle.h"
#include "../game/Entity/Placeable.h"
#include "../game/Entity/CPhysical.h"
#include "../game/Camera.h"
#include "../game/RW/RenderWare.h"
#include "../vendor/imgui/imgui.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

extern CGame* pGame;
extern CJavaWrapper* pJavaWrapper;

// Envia o destino selecionado pelo mapa customizado pelo mesmo fluxo de marcador
// que o cliente usa quando o usuário escolhe um ponto no mapa nativo.
void NexionMapSendWaypoint(float x, float y, float z);

namespace
{
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kWorldHalf = 3000.0f;
    constexpr float kWorldSize = kWorldHalf * 2.0f;
    // Área mostrada pelo círculo. Com o minimapa maior, o alcance também cresce
    // para manter a cartografia mais afastada e sem aspecto ampliado demais.
    constexpr float kOnFootRange = 600.0f;
    constexpr float kVehicleRange = 700.0f;
    // No minimapa, pontos muito distantes não são presos na borda: eles só aparecem
    // quando o jogador já está na região do local. O mapa grande continua completo.
    float g_miniMapBlipVisibilityRange = 480.0f;
    // Zoom discreto aplicado somente ao selecionar um item da legenda lateral.
    constexpr float kLegendFocusZoom = 0.56f;
    // Destinos só são aceitos dentro desta distância de uma via real. Isso evita
    // que água, praias e áreas sem rua virem marcadores ou linhas falsas.
    constexpr float kMaxDestinationRoadDistance = 60.0f;
    // Ao sair mais que esta distância da polilinha, o GPS cria um novo caminho
    // a partir da posição atual, tal como navegadores de rota fazem.
    constexpr float kRouteDeviationDistance = 35.0f;
    constexpr uint32_t kRouteRecalculationCooldownMs = 1200;
    constexpr float kDestinationArrivalDistance = 16.0f;
    constexpr int kMapWidth = 1024;
    constexpr int kMapHeight = 1024;
    constexpr int kDestinationMarkerWidth = 43;
    constexpr int kDestinationMarkerHeight = 43;
    constexpr int kServerBlipIconWidth = 32;
    constexpr int kServerBlipIconHeight = 32;
    constexpr size_t kMapBytesPerPixel = 4;
    constexpr size_t kServerBlipCount = 100;
    // O pacote visual contém blip_1 até blip_194. O tipo 0 e IDs sem sprite
    // continuam usando o ponto colorido para manter a compatibilidade SA-MP.
    constexpr size_t kServerBlipTextureCount = 195;
    // O modo de diagnóstico foi encerrado: a nova cartografia usa seu alpha original.
    constexpr bool kDiagnosticOpaqueMap = false;

    struct ServerBlip
    {
        bool active = false;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        uint8_t type = 0;
        uint32_t color = 0xFFFFFFFF;
        uint8_t style = 0;
    };

    // Pontos permanentes, exclusivamente locais. Cada novo local deve ser
    // cadastrado aqui com nome, posição GTA e tipo do ícone; a legenda lateral
    // do mapa grande é atualizada automaticamente.
    struct LocalMapBlip
    {
        const char* name = "";
        ServerBlip blip{};
    };

    std::array<ServerBlip, kServerBlipCount> g_serverBlips{};
    constexpr size_t kLocalMapBlipCount = 3;
    const std::array<LocalMapBlip, kLocalMapBlipCount> kLocalMapBlips{{
        { "Prefeitura", { true, 1481.0f, -1769.0f, 13.5f, 3, 0xFFFFFFFF, 0 } },
        // Pontos temporários para validar que itens iguais ainda possuem posição e
        // centralização independentes na legenda lateral.
        { "Teste", { true, 1230.0f, -1380.0f, 13.5f, 35, 0xFFFFFFFF, 0 } },
        { "Teste", { true, -1975.0f, 135.0f, 27.0f, 35, 0xFFFFFFFF, 0 } },
    }};

    // A legenda agrupa locais pelo mesmo nome. Cada grupo preserva todos os seus
    // índices e somente alterna qual posição está selecionada no momento.
    struct LocalMapBlipGroup
    {
        const char* name = "";
        std::array<int, kLocalMapBlipCount> localIndices{};
        int count = 0;
        int selected = 0;
    };
    std::array<LocalMapBlipGroup, kLocalMapBlipCount> g_localMapBlipGroups{};
    int g_localMapBlipGroupCount = 0;
    bool g_localMapBlipGroupsReady = false;

    void EnsureLocalMapBlipGroups()
    {
        if (g_localMapBlipGroupsReady) return;
        g_localMapBlipGroupCount = 0;
        for (int localIndex = 0; localIndex < static_cast<int>(kLocalMapBlips.size()); ++localIndex)
        {
            const LocalMapBlip& localBlip = kLocalMapBlips[static_cast<size_t>(localIndex)];
            int groupIndex = -1;
            for (int candidate = 0; candidate < g_localMapBlipGroupCount; ++candidate)
            {
                if (std::strcmp(g_localMapBlipGroups[static_cast<size_t>(candidate)].name, localBlip.name) == 0)
                {
                    groupIndex = candidate;
                    break;
                }
            }
            if (groupIndex < 0)
            {
                groupIndex = g_localMapBlipGroupCount++;
                g_localMapBlipGroups[static_cast<size_t>(groupIndex)].name = localBlip.name;
            }
            LocalMapBlipGroup& group = g_localMapBlipGroups[static_cast<size_t>(groupIndex)];
            group.localIndices[static_cast<size_t>(group.count++)] = localIndex;
        }
        g_localMapBlipGroupsReady = true;
    }
    RwTexture* g_mapTexture = nullptr;
    RwTexture* g_destinationMarkerTexture = nullptr;
    // As texturas são carregadas somente se o servidor realmente usar aquele tipo.
    // Assim, os 194 assets não consomem memória de vídeo de uma vez.
    std::array<RwTexture*, kServerBlipTextureCount> g_serverBlipTextures{};
    std::array<uint32_t, kServerBlipTextureCount> g_nextServerBlipTextureLoadAttempt{};
    uint32_t g_nextTextureLoadAttempt = 0;
    uint32_t g_nextDestinationMarkerLoadAttempt = 0;

    bool g_largeMapOpen = false;
    bool g_largeMapDragging = false;

    // O modo de mapa livre também controla a ImageView Android da logo. A chamada
    // é feita apenas quando o estado muda, não durante cada frame de renderização.
    void SetLargeMapOpenState(bool open)
    {
        if (g_largeMapOpen == open) return;
        g_largeMapOpen = open;
        if (pJavaWrapper) pJavaWrapper->SetNexionHudLogoVisible(!open);
    }
    bool g_miniMapTouchActive = false;
    ImVec2 g_largeMapLastTouch(0.0f, 0.0f);
    ImVec2 g_largeMapCenterUv(0.5f, 0.5f);
    ImVec2 g_lastDisplaySize(0.0f, 0.0f);
    // Escala inicial para a camada livre. O mapa inteiro é renderizado e deslocado
    // pela tela; não há mais uma janela UV quadrada recortando a cartografia.
    float g_largeMapZoom = 0.45f;
    constexpr int kLargeMapTouchCount = 3;
    std::array<ImVec2, kLargeMapTouchCount> g_largeMapTouchPosition{};
    std::array<ImVec2, kLargeMapTouchCount> g_largeMapTouchStart{};
    std::array<bool, kLargeMapTouchCount> g_largeMapTouchDown{};
    std::array<bool, kLargeMapTouchCount> g_largeMapTouchMoved{};
    // Índice do grupo lateral capturado por cada dedo; -1 significa toque livre no mapa.
    std::array<int, kLargeMapTouchCount> g_largeMapLegendTouchIndex{ -1, -1, -1 };
    enum LargeMapLegendTouchAction
    {
        kLegendTouchNone = 0,
        kLegendTouchCenter,
        kLegendTouchPrevious,
        kLegendTouchNext,
    };
    std::array<int, kLargeMapTouchCount> g_largeMapLegendTouchAction{};
    bool g_largeMapPinching = false;
    float g_largeMapPinchDistance = 0.0f;
    bool g_ignoreMapOpenTouchRelease = false;

    // Destino escolhido pelo jogador no mapa customizado. As coordenadas ficam em
    // UV para que mapa grande e minimapa compartilhem exatamente a mesma posição.
    bool g_destinationActive = false;
    ImVec2 g_destinationUv(0.5f, 0.5f);

    // A malha viária é imutável depois do carregamento. Os trabalhos em segundo
    // plano recebem uma cópia compartilhada dela e só publicam o resultado pela
    // fila principal do jogo, portanto o render nunca aguarda I/O ou A*.
    struct RoadEdge
    {
        int nodeId = 0;
        float distance = 0.0f;
    };

    struct RoadNode
    {
        float x = 0.0f;
        float y = 0.0f;
        std::vector<RoadEdge> neighbours;
    };

    struct RoadGraph
    {
        std::unordered_map<int, RoadNode> nodes;
    };

    struct WorldPoint
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct RouteProjection
    {
        size_t segment = 0;
        float segmentProjection = 0.0f;
        float distanceSquared = std::numeric_limits<float>::infinity();
    };

    std::shared_ptr<const RoadGraph> g_roadGraph;
    std::vector<WorldPoint> g_roadRoute;
    bool g_roadGraphLoading = false;
    bool g_routeCalculating = false;
    uint32_t g_nextRoadGraphLoadAttempt = 0;
    uint32_t g_routeGeneration = 0;
    uint32_t g_nextRouteRecalculationTick = 0;
    size_t g_routeProgressSegment = 0;

    // Estado visual exclusivo do minimapa. O alvo continua sendo calculado pelo
    // GTA a cada frame, mas zoom e orientação chegam ao desenho gradualmente para
    // eliminar saltos ao entrar/sair de veículos e ao trocar o heading da câmera.
    bool g_miniMapMotionInitialized = false;
    float g_smoothedMiniMapHeading = 0.0f;
    float g_smoothedMiniMapWorldRange = kOnFootRange;
    uint32_t g_lastMiniMapMotionTick = 0;

    CPlaceable* GetRadarSubject(CPlayerPed* playerPed, bool& inVehicle);

    class JsonCursor
    {
    public:
        JsonCursor(const char* begin, const char* end) : m_current(begin), m_end(end) {}

        bool Consume(char expected)
        {
            SkipWhitespace();
            if (m_current >= m_end || *m_current != expected) return false;
            ++m_current;
            return true;
        }

        bool ReadString(std::string& value)
        {
            SkipWhitespace();
            if (m_current >= m_end || *m_current != '\"') return false;
            ++m_current;
            value.clear();
            while (m_current < m_end && *m_current != '\"')
            {
                // O nodes.json contém apenas chaves simples. Escapes não fazem
                // parte desse formato e são rejeitados para manter o parser leve.
                if (*m_current == '\\') return false;
                value.push_back(*m_current++);
            }
            if (m_current >= m_end) return false;
            ++m_current;
            return true;
        }

        bool ReadNumber(float& value)
        {
            SkipWhitespace();
            if (m_current >= m_end) return false;
            char* parsedEnd = nullptr;
            value = std::strtof(m_current, &parsedEnd);
            if (parsedEnd == m_current || parsedEnd == nullptr) return false;
            m_current = parsedEnd;
            return true;
        }

    private:
        void SkipWhitespace()
        {
            while (m_current < m_end && std::isspace(static_cast<unsigned char>(*m_current)))
                ++m_current;
        }

        const char* m_current;
        const char* m_end;
    };

    bool ParseIntegerKey(const std::string& text, int& value)
    {
        char* parsedEnd = nullptr;
        const long parsed = std::strtol(text.c_str(), &parsedEnd, 10);
        if (parsedEnd == text.c_str() || parsedEnd == nullptr || *parsedEnd != '\0') return false;
        value = static_cast<int>(parsed);
        return true;
    }

    bool ParseNeighbours(JsonCursor& json, std::vector<RoadEdge>& neighbours)
    {
        if (!json.Consume('{')) return false;
        bool first = true;
        for (;;)
        {
            if (json.Consume('}')) return true;
            if (!first && !json.Consume(',')) return false;
            first = false;

            std::string key;
            int neighbourId = 0;
            float distance = 0.0f;
            if (!json.ReadString(key) || !ParseIntegerKey(key, neighbourId) ||
                !json.Consume(':') || !json.ReadNumber(distance))
                return false;
            neighbours.push_back({ neighbourId, distance });
        }
    }

    bool ParseRoadNode(JsonCursor& json, int& nodeId, RoadNode& node)
    {
        if (!json.Consume('{')) return false;
        bool first = true;
        bool hasId = false;
        bool hasX = false;
        bool hasY = false;
        bool hasNeighbours = false;

        for (;;)
        {
            if (json.Consume('}')) break;
            if (!first && !json.Consume(',')) return false;
            first = false;

            std::string field;
            if (!json.ReadString(field) || !json.Consume(':')) return false;
            if (field == "id")
            {
                float idValue = 0.0f;
                if (!json.ReadNumber(idValue)) return false;
                nodeId = static_cast<int>(idValue);
                hasId = true;
            }
            else if (field == "x")
            {
                if (!json.ReadNumber(node.x)) return false;
                hasX = true;
            }
            else if (field == "y")
            {
                if (!json.ReadNumber(node.y)) return false;
                hasY = true;
            }
            else if (field == "z")
            {
                float ignoredZ = 0.0f;
                if (!json.ReadNumber(ignoredZ)) return false;
            }
            else if (field == "neighbours")
            {
                if (!ParseNeighbours(json, node.neighbours)) return false;
                hasNeighbours = true;
            }
            else
            {
                return false;
            }
        }

        return hasId && hasX && hasY && hasNeighbours;
    }

    std::shared_ptr<RoadGraph> LoadRoadGraphFromFile(const std::string& assetPath)
    {
        std::FILE* file = std::fopen(assetPath.c_str(), "rb");
        if (!file) return nullptr;
        if (std::fseek(file, 0, SEEK_END) != 0)
        {
            std::fclose(file);
            return nullptr;
        }
        const long length = std::ftell(file);
        if (length <= 2 || length > 32L * 1024L * 1024L || std::fseek(file, 0, SEEK_SET) != 0)
        {
            std::fclose(file);
            return nullptr;
        }

        std::vector<char> raw(static_cast<size_t>(length) + 1, '\0');
        const bool readOk = std::fread(raw.data(), 1, static_cast<size_t>(length), file) == static_cast<size_t>(length);
        std::fclose(file);
        if (!readOk) return nullptr;

        JsonCursor json(raw.data(), raw.data() + length);
        if (!json.Consume('{')) return nullptr;

        auto graph = std::make_shared<RoadGraph>();
        graph->nodes.reserve(70000);
        bool first = true;
        for (;;)
        {
            if (json.Consume('}')) break;
            if (!first && !json.Consume(',')) return nullptr;
            first = false;

            std::string outerKey;
            int outerId = 0;
            int nodeId = 0;
            RoadNode node;
            if (!json.ReadString(outerKey) || !ParseIntegerKey(outerKey, outerId) ||
                !json.Consume(':') || !ParseRoadNode(json, nodeId, node) || outerId != nodeId)
                return nullptr;
            graph->nodes.emplace(nodeId, std::move(node));
        }

        return graph->nodes.empty() ? nullptr : graph;
    }

    float DistanceSquared(float ax, float ay, float bx, float by)
    {
        const float dx = ax - bx;
        const float dy = ay - by;
        return dx * dx + dy * dy;
    }

    float DistanceBetween(const RoadNode& first, const RoadNode& second)
    {
        return std::sqrt(DistanceSquared(first.x, first.y, second.x, second.y));
    }

    const RoadNode* FindNearestRoadNode(const RoadGraph& graph, float worldX, float worldY,
                                        int& nodeId, float& distanceSquared)
    {
        nodeId = -1;
        distanceSquared = std::numeric_limits<float>::infinity();
        const RoadNode* nearest = nullptr;
        for (const auto& item : graph.nodes)
        {
            const RoadNode& node = item.second;
            const float distance = DistanceSquared(node.x, node.y, worldX, worldY);
            if (distance >= distanceSquared) continue;
            distanceSquared = distance;
            nodeId = item.first;
            nearest = &node;
        }
        return nearest;
    }

    std::vector<WorldPoint> FindRoadRoute(const RoadGraph& graph, float startX, float startY, float targetX, float targetY)
    {
        int startId = -1;
        int targetId = -1;
        float nearestStart = 0.0f;
        float nearestTarget = 0.0f;
        const RoadNode* startNode = FindNearestRoadNode(graph, startX, startY, startId, nearestStart);
        const RoadNode* targetNodePointer = FindNearestRoadNode(graph, targetX, targetY, targetId, nearestTarget);
        if (!startNode || !targetNodePointer) return {};

        using QueueEntry = std::pair<float, int>;
        std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> open;
        std::unordered_map<int, float> score;
        std::unordered_map<int, int> previous;
        score.reserve(graph.nodes.size() / 2);
        previous.reserve(graph.nodes.size() / 2);

        const RoadNode& targetNode = *targetNodePointer;
        score.emplace(startId, 0.0f);
        open.emplace(DistanceBetween(*startNode, targetNode), startId);
        bool reachedTarget = startId == targetId;

        while (!open.empty() && !reachedTarget)
        {
            const QueueEntry entry = open.top();
            open.pop();
            const int currentId = entry.second;
            const auto currentScore = score.find(currentId);
            const auto currentNode = graph.nodes.find(currentId);
            if (currentScore == score.end() || currentNode == graph.nodes.end()) continue;

            const float expected = currentScore->second + DistanceBetween(currentNode->second, targetNode);
            if (entry.first > expected + 0.01f) continue;
            if (currentId == targetId)
            {
                reachedTarget = true;
                break;
            }

            for (const RoadEdge& edge : currentNode->second.neighbours)
            {
                const auto nextNode = graph.nodes.find(edge.nodeId);
                if (nextNode == graph.nodes.end()) continue;
                const float nextScore = currentScore->second + edge.distance;
                const auto oldScore = score.find(edge.nodeId);
                if (oldScore != score.end() && nextScore >= oldScore->second) continue;
                previous[edge.nodeId] = currentId;
                score[edge.nodeId] = nextScore;
                open.emplace(nextScore + DistanceBetween(nextNode->second, targetNode), edge.nodeId);
            }
        }

        if (!reachedTarget) return {};

        std::vector<int> reverseIds;
        reverseIds.reserve(1024);
        for (int current = targetId; current != startId;)
        {
            reverseIds.push_back(current);
            const auto before = previous.find(current);
            if (before == previous.end()) return {};
            current = before->second;
        }
        reverseIds.push_back(startId);

        std::vector<WorldPoint> route;
        route.reserve(reverseIds.size() + 2);
        route.push_back({ startX, startY });
        for (auto it = reverseIds.rbegin(); it != reverseIds.rend(); ++it)
        {
            const RoadNode& node = graph.nodes.at(*it);
            route.push_back({ node.x, node.y });
        }
        route.push_back({ targetX, targetY });
        return route;
    }

    void StartRoadRouteCalculationForDestination();

    void EnsureRoadGraphLoad()
    {
        if (g_roadGraph || g_roadGraphLoading || !g_pszStorage) return;
        const uint32_t now = GetTickCount();
        if (now < g_nextRoadGraphLoadAttempt) return;

        char path[512]{};
        std::snprintf(path, sizeof(path), "%sSAMP/nexion_radar/nodes.json", g_pszStorage);
        const std::string assetPath(path);
        g_roadGraphLoading = true;
        std::thread([assetPath]() {
            std::shared_ptr<RoadGraph> graph = LoadRoadGraphFromFile(assetPath);
            CGame::PostToMainThread([graph]() {
                g_roadGraphLoading = false;
                if (!graph)
                {
                    g_nextRoadGraphLoadAttempt = GetTickCount() + 5000;
                    FLog("NexionMiniMap: nao foi possivel carregar nodes.json");
                    return;
                }

                g_roadGraph = graph;
                FLog("NexionMiniMap: malha GPS carregada (%zu nos)", graph->nodes.size());
                if (g_destinationActive)
                    StartRoadRouteCalculationForDestination();
            });
        }).detach();
    }

    void StartRoadRouteCalculationForDestination()
    {
        if (!g_destinationActive || !g_roadGraph || !pGame) return;

        CPlayerPed* playerPed = pGame->FindPlayerPed();
        bool inVehicle = false;
        CPlaceable* subject = GetRadarSubject(playerPed, inVehicle);
        if (!subject) return;

        const CVector& position = subject->GetPosition();
        const float targetX = g_destinationUv.x * kWorldSize - kWorldHalf;
        const float targetY = kWorldHalf - g_destinationUv.y * kWorldSize;
        const uint32_t requestGeneration = ++g_routeGeneration;
        g_nextRouteRecalculationTick = GetTickCount() + kRouteRecalculationCooldownMs;
        const std::shared_ptr<const RoadGraph> graph = g_roadGraph;
        g_roadRoute.clear();
        g_routeProgressSegment = 0;
        g_routeCalculating = true;

        std::thread([graph, requestGeneration, startX = position.x, startY = position.y, targetX, targetY]() {
            std::vector<WorldPoint> route = FindRoadRoute(*graph, startX, startY, targetX, targetY);
            CGame::PostToMainThread([route = std::move(route), requestGeneration]() mutable {
                if (requestGeneration != g_routeGeneration) return;
                g_routeCalculating = false;
                if (!g_destinationActive)
                {
                    g_roadRoute.clear();
                    return;
                }
                g_roadRoute = std::move(route);
                g_routeProgressSegment = 0;
            });
        }).detach();
    }

    bool FindClosestRemainingRouteProjection(float playerX, float playerY, RouteProjection& result)
    {
        if (g_roadRoute.size() < 2) return false;

        const size_t lastSegment = g_roadRoute.size() - 2;
        const size_t firstSegment = std::min(g_routeProgressSegment, lastSegment);
        // A busca é limitada aos próximos segmentos. Assim, se duas ruas se
        // cruzarem no mapa, o GPS não pula para uma volta futura da própria rota.
        // Otimizacao: reduzido de 128 para 64 segmentos para projecao da rota.
        const size_t finalSegment = std::min(lastSegment, firstSegment + 64);
        result.segment = firstSegment;
        result.segmentProjection = 0.0f;
        result.distanceSquared = std::numeric_limits<float>::infinity();
        for (size_t index = firstSegment; index <= finalSegment; ++index)
        {
            const WorldPoint& a = g_roadRoute[index];
            const WorldPoint& b = g_roadRoute[index + 1];
            const float dx = b.x - a.x;
            const float dy = b.y - a.y;
            const float lengthSquared = dx * dx + dy * dy;
            const float projection = lengthSquared > 0.001f
                ? std::clamp(((playerX - a.x) * dx + (playerY - a.y) * dy) / lengthSquared, 0.0f, 1.0f)
                : 0.0f;
            const float projectedX = a.x + dx * projection;
            const float projectedY = a.y + dy * projection;
            const float distance = DistanceSquared(playerX, playerY, projectedX, projectedY);
            if (distance >= result.distanceSquared) continue;
            result.distanceSquared = distance;
            result.segment = index;
            result.segmentProjection = projection;
        }
        return std::isfinite(result.distanceSquared);
    }

    void ClearActiveDestination()
    {
        g_destinationActive = false;
        g_roadRoute.clear();
        g_routeProgressSegment = 0;
        g_routeCalculating = false;
        g_nextRouteRecalculationTick = 0;
        ++g_routeGeneration;
    }

    bool MaybeClearArrivedDestination(float playerX, float playerY)
    {
        if (!g_destinationActive) return false;
        const float targetX = g_destinationUv.x * kWorldSize - kWorldHalf;
        const float targetY = kWorldHalf - g_destinationUv.y * kWorldSize;
        const float arrivalDistanceSquared = kDestinationArrivalDistance * kDestinationArrivalDistance;
        if (DistanceSquared(playerX, playerY, targetX, targetY) > arrivalDistanceSquared) return false;

        ClearActiveDestination();
        FLog("NexionMiniMap: destino alcancado, rota encerrada");
        return true;
    }

    void MaybeRecalculateRoadRoute(float playerX, float playerY)
    {
        if (!g_destinationActive || g_routeCalculating || !g_roadGraph || g_roadRoute.size() < 2) return;

        RouteProjection projection;
        if (!FindClosestRemainingRouteProjection(playerX, playerY, projection)) return;
        const float deviationSquared = kRouteDeviationDistance * kRouteDeviationDistance;
        if (projection.distanceSquared <= deviationSquared) return;

        const uint32_t now = GetTickCount();
        if (now < g_nextRouteRecalculationTick) return;
        g_nextRouteRecalculationTick = now + kRouteRecalculationCooldownMs;
        StartRoadRouteCalculationForDestination();
    }

    WorldPoint GetMiniMapGuidanceTarget(float playerX, float playerY, float targetX, float targetY)
    {
        if (g_roadRoute.size() < 2) return { targetX, targetY };

        RouteProjection projection;
        if (!FindClosestRemainingRouteProjection(playerX, playerY, projection)) return { targetX, targetY };
        const size_t lastSegment = g_roadRoute.size() - 2;
        g_routeProgressSegment = std::max(g_routeProgressSegment, projection.segment);
        if (projection.segmentProjection > 0.82f && g_routeProgressSegment < lastSegment)
            ++g_routeProgressSegment;
        return g_roadRoute[std::min(g_routeProgressSegment + 1, g_roadRoute.size() - 1)];
    }

    ImU32 ToImGuiColor(uint32_t sampColor)
    {
        const uint8_t red = static_cast<uint8_t>((sampColor >> 24) & 0xFF);
        const uint8_t green = static_cast<uint8_t>((sampColor >> 16) & 0xFF);
        const uint8_t blue = static_cast<uint8_t>((sampColor >> 8) & 0xFF);
        uint8_t alpha = static_cast<uint8_t>(sampColor & 0xFF);
        if (alpha == 0) alpha = 255;
        return IM_COL32(red, green, blue, alpha);
    }

    bool EnsureMapTexture()
    {
        if (g_mapTexture && g_mapTexture->raster)
            return true;

        const uint32_t now = GetTickCount();
        if (now < g_nextTextureLoadAttempt)
            return false;

        g_nextTextureLoadAttempt = now + 30000; // Otimizacao: 30s em vez de 2.5s para evitar I/O na thread de render.
        if (!g_pszStorage || !RwImageCreate || !RwImageAllocatePixels || !RwImageDestroy ||
            !RwRasterCreate || !RwRasterSetFromImage || !RwTextureCreate)
            return false;

        char assetPath[512]{};
        std::snprintf(assetPath, sizeof(assetPath), "%sSAMP/nexion_radar/map_minimap.rgba", g_pszStorage);

        std::FILE* asset = std::fopen(assetPath, "rb");
        if (!asset)
        {
            FLog("NexionMiniMap: asset RGBA ainda indisponivel: %s", assetPath);
            return false;
        }

        RwImage* image = RwImageCreate(kMapWidth, kMapHeight, 32);
        if (!image || !RwImageAllocatePixels(image) || !image->cpPixels)
        {
            if (image) RwImageDestroy(image);
            std::fclose(asset);
            FLog("NexionMiniMap: falha ao alocar imagem RGBA");
            return false;
        }

        bool readSucceeded = true;
        const size_t rowBytes = static_cast<size_t>(kMapWidth) * kMapBytesPerPixel;
        for (int row = 0; row < kMapHeight; ++row)
        {
            uint8_t* rowPixels = image->cpPixels + static_cast<size_t>(row) * image->stride;
            if (std::fread(rowPixels, 1, rowBytes, asset) != rowBytes)
            {
                readSucceeded = false;
                break;
            }
        }
        std::fclose(asset);

        if (!readSucceeded)
        {
            RwImageDestroy(image);
            FLog("NexionMiniMap: asset RGBA incompleto: %s", assetPath);
            return false;
        }

        if (kDiagnosticOpaqueMap)
        {
            for (int row = 0; row < kMapHeight; ++row)
            {
                uint8_t* rowPixels = image->cpPixels + static_cast<size_t>(row) * image->stride;
                for (int column = 0; column < kMapWidth; ++column)
                    rowPixels[static_cast<size_t>(column) * kMapBytesPerPixel + 3] = 255;
            }
            FLog("NexionMiniMap: modo diagnostico opaco ativo");
        }

        RwRaster* raster = RwRasterCreate(kMapWidth, kMapHeight, 32, rwRASTERFORMAT8888 | rwRASTERTYPETEXTURE);
        if (!raster || !RwRasterSetFromImage(raster, image))
        {
            if (raster && RwRasterDestroy) RwRasterDestroy(raster);
            RwImageDestroy(image);
            FLog("NexionMiniMap: falha ao criar raster %dx%d", kMapWidth, kMapHeight);
            return false;
        }

        RwImageDestroy(image);

        g_mapTexture = RwTextureCreate(raster);
        if (!g_mapTexture)
        {
            if (RwRasterDestroy) RwRasterDestroy(raster);
            FLog("NexionMiniMap: falha ao criar textura");
            return false;
        }

        RwTextureSetFilterMode(g_mapTexture, rwFILTERLINEAR);
        FLog("NexionMiniMap: textura RGBA carregada %dx%d", kMapWidth, kMapHeight);
        return true;
    }

    bool EnsureDestinationMarkerTexture()
    {
        if (g_destinationMarkerTexture && g_destinationMarkerTexture->raster)
            return true;

        const uint32_t now = GetTickCount();
        if (now < g_nextDestinationMarkerLoadAttempt)
            return false;

        g_nextDestinationMarkerLoadAttempt = now + 30000; // Otimizacao: 30s em vez de 2.5s.
        if (!g_pszStorage || !RwImageCreate || !RwImageAllocatePixels || !RwImageDestroy ||
            !RwRasterCreate || !RwRasterSetFromImage || !RwTextureCreate)
            return false;

        char assetPath[512]{};
        std::snprintf(assetPath, sizeof(assetPath), "%sSAMP/nexion_radar/marker.rgba", g_pszStorage);
        std::FILE* asset = std::fopen(assetPath, "rb");
        if (!asset)
            return false;

        RwImage* image = RwImageCreate(kDestinationMarkerWidth, kDestinationMarkerHeight, 32);
        if (!image || !RwImageAllocatePixels(image) || !image->cpPixels)
        {
            if (image) RwImageDestroy(image);
            std::fclose(asset);
            return false;
        }

        const size_t rowBytes = static_cast<size_t>(kDestinationMarkerWidth) * kMapBytesPerPixel;
        bool readSucceeded = true;
        for (int row = 0; row < kDestinationMarkerHeight; ++row)
        {
            uint8_t* rowPixels = image->cpPixels + static_cast<size_t>(row) * image->stride;
            if (std::fread(rowPixels, 1, rowBytes, asset) != rowBytes)
            {
                readSucceeded = false;
                break;
            }
        }
        std::fclose(asset);

        if (!readSucceeded)
        {
            RwImageDestroy(image);
            return false;
        }

        RwRaster* raster = RwRasterCreate(kDestinationMarkerWidth, kDestinationMarkerHeight, 32,
                                          rwRASTERFORMAT8888 | rwRASTERTYPETEXTURE);
        if (!raster || !RwRasterSetFromImage(raster, image))
        {
            if (raster && RwRasterDestroy) RwRasterDestroy(raster);
            RwImageDestroy(image);
            return false;
        }
        RwImageDestroy(image);

        g_destinationMarkerTexture = RwTextureCreate(raster);
        if (!g_destinationMarkerTexture)
        {
            if (RwRasterDestroy) RwRasterDestroy(raster);
            return false;
        }

        RwTextureSetFilterMode(g_destinationMarkerTexture, rwFILTERLINEAR);
        FLog("NexionMiniMap: marcador de destino carregado");
        return true;
    }

    void DrawDestinationMarker(ImDrawList* draw, const ImVec2& position, float size)
    {
        if (!draw || !g_destinationMarkerTexture || !g_destinationMarkerTexture->raster) return;
        const float half = size * 0.5f;
        draw->AddImage(reinterpret_cast<ImTextureID>(g_destinationMarkerTexture->raster),
                       ImVec2(position.x - half, position.y - half),
                       ImVec2(position.x + half, position.y + half),
                       ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32_WHITE);
    }

    RwTexture* EnsureServerBlipTexture(uint8_t type)
    {
        if (type == 0 || type >= kServerBlipTextureCount)
            return nullptr;

        RwTexture*& texture = g_serverBlipTextures[type];
        if (texture && texture->raster)
            return texture;

        const uint32_t now = GetTickCount();
        if (now < g_nextServerBlipTextureLoadAttempt[type])
            return nullptr;
        // Otimizacao: aumentado de 2.5s para 30s. O I/O de fopen/fread trava a
        // thread de render quando o asset nao existe; espacar as tentativas
        // evita micro-stutters repetidos por blips ausentes.
        g_nextServerBlipTextureLoadAttempt[type] = now + 30000;

        if (!g_pszStorage || !RwImageCreate || !RwImageAllocatePixels || !RwImageDestroy ||
            !RwRasterCreate || !RwRasterSetFromImage || !RwTextureCreate)
            return nullptr;

        char assetPath[512]{};
        std::snprintf(assetPath, sizeof(assetPath), "%sSAMP/nexion_radar/blips/blip_%u.rgba", g_pszStorage,
                      static_cast<unsigned int>(type));
        std::FILE* asset = std::fopen(assetPath, "rb");
        if (!asset)
            return nullptr;

        RwImage* image = RwImageCreate(kServerBlipIconWidth, kServerBlipIconHeight, 32);
        if (!image || !RwImageAllocatePixels(image) || !image->cpPixels)
        {
            if (image) RwImageDestroy(image);
            std::fclose(asset);
            return nullptr;
        }

        const size_t rowBytes = static_cast<size_t>(kServerBlipIconWidth) * kMapBytesPerPixel;
        bool readSucceeded = true;
        for (int row = 0; row < kServerBlipIconHeight; ++row)
        {
            uint8_t* rowPixels = image->cpPixels + static_cast<size_t>(row) * image->stride;
            if (std::fread(rowPixels, 1, rowBytes, asset) != rowBytes)
            {
                readSucceeded = false;
                break;
            }
        }
        std::fclose(asset);

        if (!readSucceeded)
        {
            RwImageDestroy(image);
            return nullptr;
        }

        RwRaster* raster = RwRasterCreate(kServerBlipIconWidth, kServerBlipIconHeight, 32,
                                          rwRASTERFORMAT8888 | rwRASTERTYPETEXTURE);
        if (!raster || !RwRasterSetFromImage(raster, image))
        {
            if (raster && RwRasterDestroy) RwRasterDestroy(raster);
            RwImageDestroy(image);
            return nullptr;
        }
        RwImageDestroy(image);

        texture = RwTextureCreate(raster);
        if (!texture)
        {
            if (RwRasterDestroy) RwRasterDestroy(raster);
            return nullptr;
        }
        RwTextureSetFilterMode(texture, rwFILTERLINEAR);
        FLog("NexionMiniMap: blip %u carregado", static_cast<unsigned int>(type));
        return texture;
    }

    void DrawServerBlip(ImDrawList* draw, const ServerBlip& blip, const ImVec2& position,
                        float texturedSize, float fallbackRadius)
    {
        if (!draw) return;

        RwTexture* texture = EnsureServerBlipTexture(blip.type);
        if (texture && texture->raster)
        {
            const float half = texturedSize * 0.5f;
            draw->AddImage(reinterpret_cast<ImTextureID>(texture->raster),
                           ImVec2(position.x - half, position.y - half),
                           ImVec2(position.x + half, position.y + half),
                           ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ToImGuiColor(blip.color));
            return;
        }

        draw->AddCircleFilled(position, fallbackRadius, IM_COL32(12, 14, 18, 220), 12);
        draw->AddCircleFilled(position, fallbackRadius - 1.3f, ToImGuiColor(blip.color), 12);
    }

    struct LargeMapLegendLayout
    {
        ImVec2 rowStart{};
        ImVec2 rowEnd{};
        ImVec2 previousStart{};
        ImVec2 previousEnd{};
        ImVec2 nextStart{};
        ImVec2 nextEnd{};
        ImVec2 counterStart{};
        ImVec2 counterEnd{};
        bool hasPages = false;
    };

    bool GetLargeMapLegendLayout(int groupIndex, const ImVec2& display, float scale,
                                 LargeMapLegendLayout& layout)
    {
        EnsureLocalMapBlipGroups();
        const float marginRight = 32.0f * scale;
        const float top = 70.0f * scale;
        const float rowHeight = 56.0f * scale;
        const float rowGap = 9.0f * scale;
        const float bottomLimit = display.y - 36.0f * scale;
        const int maxRows = std::max(1, static_cast<int>((bottomLimit - top + rowGap) / (rowHeight + rowGap)));
        const int rowCount = std::min(g_localMapBlipGroupCount, maxRows);
        if (groupIndex < 0 || groupIndex >= rowCount) return false;

        const LocalMapBlipGroup& group = g_localMapBlipGroups[static_cast<size_t>(groupIndex)];
        const bool hasPages = group.count > 1;
        const float fontSize = 22.0f * scale;
        const float leftPadding = 16.0f * scale;
        const float iconSize = 31.0f * scale;
        const float iconGap = 14.0f * scale;
        const float rightPadding = 24.0f * scale;
        const float minWidth = 112.0f * scale;
        const float maxWidth = std::max(minWidth, display.x - marginRight - 12.0f * scale);
        const ImVec2 fullTextSize = ImGui::GetFont()->CalcTextSizeA(fontSize,
                                                                     std::numeric_limits<float>::max(),
                                                                     0.0f, group.name);
        char counterText[16]{};
        float pagesWidth = 0.0f;
        if (hasPages)
        {
            std::snprintf(counterText, sizeof(counterText), "%d/%d", group.selected + 1, group.count);
            const float counterWidth = ImGui::GetFont()->CalcTextSizeA(fontSize,
                                                                         std::numeric_limits<float>::max(),
                                                                         0.0f, counterText).x;
            pagesWidth = 18.0f * scale + 8.0f * scale + counterWidth + 8.0f * scale + 18.0f * scale;
        }
        const float naturalWidth = leftPadding + fullTextSize.x + (hasPages ? 12.0f * scale + pagesWidth : 0.0f)
                                   + iconGap + iconSize + rightPadding;
        const float panelWidth = std::clamp(naturalWidth, minWidth, maxWidth);
        const float panelRight = display.x - marginRight;
        const float rowTop = top + static_cast<float>(groupIndex) * (rowHeight + rowGap);
        layout.rowStart = ImVec2(panelRight - panelWidth, rowTop);
        layout.rowEnd = ImVec2(panelRight, rowTop + rowHeight);
        layout.hasPages = hasPages;

        if (hasPages)
        {
            const float arrowWidth = 18.0f * scale;
            const float controlGap = 8.0f * scale;
            const float counterWidth = ImGui::GetFont()->CalcTextSizeA(fontSize,
                                                                         std::numeric_limits<float>::max(),
                                                                         0.0f, counterText).x;
            const float iconLeft = layout.rowEnd.x - 24.0f * scale - iconSize * 0.5f;
            layout.nextEnd = ImVec2(iconLeft - iconGap, layout.rowEnd.y);
            layout.nextStart = ImVec2(layout.nextEnd.x - arrowWidth, layout.rowStart.y);
            layout.counterEnd = ImVec2(layout.nextStart.x - controlGap, layout.rowEnd.y);
            layout.counterStart = ImVec2(layout.counterEnd.x - counterWidth, layout.rowStart.y);
            layout.previousEnd = ImVec2(layout.counterStart.x - controlGap, layout.rowEnd.y);
            layout.previousStart = ImVec2(layout.previousEnd.x - arrowWidth, layout.rowStart.y);
        }
        return true;
    }

    bool IsPointInsideRect(const ImVec2& point, const ImVec2& start, const ImVec2& end)
    {
        return point.x >= start.x && point.x <= end.x && point.y >= start.y && point.y <= end.y;
    }

    int FindLargeMapLegendItem(const ImVec2& touch, const ImVec2& display, float scale)
    {
        EnsureLocalMapBlipGroups();
        for (int groupIndex = 0; groupIndex < g_localMapBlipGroupCount; ++groupIndex)
        {
            LargeMapLegendLayout layout;
            if (!GetLargeMapLegendLayout(groupIndex, display, scale, layout)) continue;
            if (IsPointInsideRect(touch, layout.rowStart, layout.rowEnd)) return groupIndex;
        }
        return -1;
    }

    int GetLargeMapLegendTouchAction(int groupIndex, const ImVec2& touch, const ImVec2& display, float scale)
    {
        LargeMapLegendLayout layout;
        if (!GetLargeMapLegendLayout(groupIndex, display, scale, layout)) return kLegendTouchNone;
        if (!layout.hasPages) return kLegendTouchCenter;
        if (IsPointInsideRect(touch, layout.previousStart, layout.previousEnd)) return kLegendTouchPrevious;
        if (IsPointInsideRect(touch, layout.nextStart, layout.nextEnd)) return kLegendTouchNext;
        return kLegendTouchCenter;
    }

    int GetSelectedLocalMapBlipIndex(int groupIndex)
    {
        EnsureLocalMapBlipGroups();
        if (groupIndex < 0 || groupIndex >= g_localMapBlipGroupCount) return -1;
        const LocalMapBlipGroup& group = g_localMapBlipGroups[static_cast<size_t>(groupIndex)];
        if (group.count <= 0 || group.selected < 0 || group.selected >= group.count) return -1;
        return group.localIndices[static_cast<size_t>(group.selected)];
    }

    // Estas rotinas são definidas junto à lógica de zoom do mapa grande abaixo.
    void GetLargeMapSpans(const ImVec2& display, float& halfSpanX, float& halfSpanY);
    void ClampLargeMapCenter(float halfSpanX, float halfSpanY);

    void CenterLargeMapOnLocalBlip(int index)
    {
        if (index < 0 || index >= static_cast<int>(kLocalMapBlips.size())) return;
        const ServerBlip& blip = kLocalMapBlips[static_cast<size_t>(index)].blip;
        // Aproxima só um pouco o mapa ao focar um local. Se o jogador já deu um
        // zoom maior manualmente, o valor dele é preservado.
        g_largeMapZoom = std::max(g_largeMapZoom, kLegendFocusZoom);
        g_largeMapCenterUv.x = std::clamp((blip.x + kWorldHalf) / kWorldSize, 0.0f, 1.0f);
        g_largeMapCenterUv.y = std::clamp((kWorldHalf - blip.y) / kWorldSize, 0.0f, 1.0f);
        float halfSpanX = 0.0f;
        float halfSpanY = 0.0f;
        GetLargeMapSpans(g_lastDisplaySize, halfSpanX, halfSpanY);
        ClampLargeMapCenter(halfSpanX, halfSpanY);
    }

    void DrawLargeMapLegend(ImDrawList* draw, const ImVec2& display, float scale)
    {
        if (!draw || kLocalMapBlips.empty()) return;
        EnsureLocalMapBlipGroups();

        for (int groupIndex = 0; groupIndex < g_localMapBlipGroupCount; ++groupIndex)
        {
            LargeMapLegendLayout layout;
            if (!GetLargeMapLegendLayout(groupIndex, display, scale, layout)) break;
            const LocalMapBlipGroup& group = g_localMapBlipGroups[static_cast<size_t>(groupIndex)];
            const int localIndex = group.localIndices[static_cast<size_t>(group.selected)];
            const LocalMapBlip& localBlip = kLocalMapBlips[static_cast<size_t>(localIndex)];
            const float rowHeight = layout.rowEnd.y - layout.rowStart.y;
            draw->AddRectFilled(layout.rowStart, layout.rowEnd, IM_COL32(12, 14, 19, 214), 10.0f * scale);

            const float iconSize = 31.0f * scale;
            const ImVec2 iconPosition(layout.rowEnd.x - 24.0f * scale, layout.rowStart.y + rowHeight * 0.5f);
            DrawServerBlip(draw, localBlip.blip, iconPosition, iconSize, 6.5f * scale);

            const float textLeft = layout.rowStart.x + 16.0f * scale;
            const float textRight = layout.hasPages ? layout.previousStart.x - 12.0f * scale
                                                     : iconPosition.x - iconSize * 0.5f - 14.0f * scale;
            const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(22.0f * scale,
                                                                       std::numeric_limits<float>::max(),
                                                                       0.0f, group.name);
            const ImVec2 textPosition(std::max(textLeft, textRight - textSize.x),
                                      layout.rowStart.y + (rowHeight - textSize.y) * 0.5f - 1.0f * scale);
            draw->AddText(ImGui::GetFont(), 22.0f * scale,
                          ImVec2(textPosition.x + 1.4f * scale, textPosition.y + 1.4f * scale),
                          IM_COL32(0, 0, 0, 190), group.name);
            draw->AddText(ImGui::GetFont(), 22.0f * scale, textPosition,
                          IM_COL32(245, 247, 250, 255), group.name);

            if (!layout.hasPages) continue;
            char counterText[16]{};
            std::snprintf(counterText, sizeof(counterText), "%d/%d", group.selected + 1, group.count);
            const ImVec2 counterSize = ImGui::GetFont()->CalcTextSizeA(20.0f * scale,
                                                                          std::numeric_limits<float>::max(),
                                                                          0.0f, counterText);
            const ImVec2 counterPosition((layout.counterStart.x + layout.counterEnd.x - counterSize.x) * 0.5f,
                                         layout.rowStart.y + (rowHeight - counterSize.y) * 0.5f - 1.0f * scale);
            draw->AddText(ImGui::GetFont(), 20.0f * scale, counterPosition,
                          IM_COL32(220, 223, 229, 255), counterText);

            const float centerY = layout.rowStart.y + rowHeight * 0.5f;
            const ImU32 previousColor = group.selected > 0 ? IM_COL32(245, 247, 250, 255) : IM_COL32(117, 122, 132, 180);
            const ImU32 nextColor = group.selected + 1 < group.count ? IM_COL32(245, 247, 250, 255) : IM_COL32(117, 122, 132, 180);
            // A seta da área esquerda aponta para trás (<) e a da área direita
            // aponta para frente (>). As áreas de toque permanecem inalteradas.
            draw->AddTriangleFilled(ImVec2(layout.previousStart.x, centerY),
                                    ImVec2(layout.previousEnd.x, centerY - 7.0f * scale),
                                    ImVec2(layout.previousEnd.x, centerY + 7.0f * scale), previousColor);
            draw->AddTriangleFilled(ImVec2(layout.nextEnd.x, centerY),
                                    ImVec2(layout.nextStart.x, centerY - 7.0f * scale),
                                    ImVec2(layout.nextStart.x, centerY + 7.0f * scale), nextColor);
        }
    }

    CPlaceable* GetRadarSubject(CPlayerPed* playerPed, bool& inVehicle)
    {
        inVehicle = false;
        if (!playerPed || !playerPed->m_pPed)
            return nullptr;

        if (playerPed->IsInVehicle())
        {
            CVehicle* vehicle = playerPed->GetCurrentVehicle();
            if (vehicle && vehicle->m_pVehicle)
            {
                inVehicle = true;
                return vehicle->m_pVehicle;
            }
        }

        return playerPed->m_pPed;
    }

    float GetMiniMapHeading(CPlaceable* subject)
    {
        if (!subject) return 0.0f;

        // Enquanto há deslocamento, o heading da entidade é a referência estável
        // já usada pelo radar. Parado, o jogador pode girar a câmera sem alterar a
        // matriz do ped/veículo; nesse caso, usa-se o vetor frontal da câmera ativa.
        float heading = subject->GetHeading();
        CPhysical* physical = static_cast<CPhysical*>(subject);
        const CVector& movement = physical->GetMoveSpeed();
        const float horizontalSpeedSquared = movement.x * movement.x + movement.y * movement.y;
        constexpr float kStationarySpeedSquared = 0.0004f;
        if (horizontalSpeedSquared > kStationarySpeedSquared)
            return heading;

        const CVector& cameraFront = CCamera::GetActiveCamera().Front;
        const float cameraHorizontalSquared = cameraFront.x * cameraFront.x + cameraFront.y * cameraFront.y;
        if (cameraHorizontalSquared > 0.0001f)
        {
            // Mesma convenção de CPlaceable::GetHeading(): atan2(-forward.x, forward.y).
            heading = std::atan2(-cameraFront.x, cameraFront.y);
        }
        return heading;
    }

    void GetSmoothedMiniMapMotion(float targetHeading, bool inVehicle, float& heading, float& worldRange)
    {
        const float targetRange = inVehicle ? kVehicleRange : kOnFootRange;
        const uint32_t now = GetTickCount();

        // Primeiro frame, retorno após carregamento ou pausa longa: parte direto do
        // estado real para não trazer um movimento antigo para uma nova sessão.
        if (!g_miniMapMotionInitialized || now - g_lastMiniMapMotionTick > 250)
        {
            g_miniMapMotionInitialized = true;
            g_smoothedMiniMapHeading = targetHeading;
            g_smoothedMiniMapWorldRange = targetRange;
            g_lastMiniMapMotionTick = now;
            heading = targetHeading;
            worldRange = targetRange;
            return;
        }

        const float deltaSeconds = std::min(0.10f, static_cast<float>(now - g_lastMiniMapMotionTick) * 0.001f);
        g_lastMiniMapMotionTick = now;

        // A escala usa uma resposta um pouco mais lenta para a mudança a pé/veículo;
        // heading é mais responsivo, porém não dá saltos quando câmera ou entidade
        // recebe uma nova orientação.
        const float zoomBlend = 1.0f - std::exp(-3.0f * deltaSeconds);
        const float headingBlend = 1.0f - std::exp(-10.0f * deltaSeconds);
        g_smoothedMiniMapWorldRange += (targetRange - g_smoothedMiniMapWorldRange) * zoomBlend;

        float shortestAngle = std::fmod(targetHeading - g_smoothedMiniMapHeading + kPi, kPi * 2.0f);
        if (shortestAngle < 0.0f) shortestAngle += kPi * 2.0f;
        shortestAngle -= kPi;
        g_smoothedMiniMapHeading += shortestAngle * headingBlend;

        heading = g_smoothedMiniMapHeading;
        worldRange = g_smoothedMiniMapWorldRange;
    }

    void DrawMapTexture(ImDrawList* draw, const ImVec2& center, float radius, float mapU, float mapV, float heading, float worldRange)
    {
        // Otimizacao: reduzido de 40x4=160 quads para 28x3=84 quads (~47% menos draw calls).
        constexpr int kSegments = 28;
        constexpr int kRings = 3;
        const float sinHeading = std::sin(heading);
        const float cosHeading = std::cos(heading);
        const float worldToUv = worldRange / kWorldSize;
        const ImTextureID texture = reinterpret_cast<ImTextureID>(g_mapTexture->raster);

        // Precomputa seno/cosseno de cada angulo uma unica vez.
        float cosSeg[kSegments + 1];
        float sinSeg[kSegments + 1];
        constexpr float kAngleStep = (2.0f * kPi) / static_cast<float>(kSegments);
        for (int i = 0; i <= kSegments; ++i)
        {
            const float a = -kPi + kAngleStep * static_cast<float>(i);
            cosSeg[i] = std::cos(a);
            sinSeg[i] = std::sin(a);
        }

        for (int ring = 0; ring < kRings; ++ring)
        {
            const float inner = radius * static_cast<float>(ring) / static_cast<float>(kRings);
            const float outer = radius * static_cast<float>(ring + 1) / static_cast<float>(kRings);

            for (int segment = 0; segment < kSegments; ++segment)
            {
                const ImVec2 p0(center.x + cosSeg[segment] * inner, center.y + sinSeg[segment] * inner);
                const ImVec2 p1(center.x + cosSeg[segment] * outer, center.y + sinSeg[segment] * outer);
                const ImVec2 p2(center.x + cosSeg[segment + 1] * outer, center.y + sinSeg[segment + 1] * outer);
                const ImVec2 p3(center.x + cosSeg[segment + 1] * inner, center.y + sinSeg[segment + 1] * inner);

                const float qx0 = (p0.x - center.x) / radius;
                const float qy0 = (p0.y - center.y) / radius;
                const ImVec2 uv0(std::clamp(mapU + (cosHeading * qx0 + sinHeading * qy0) * worldToUv, 0.0f, 1.0f),
                                 std::clamp(mapV + (-sinHeading * qx0 + cosHeading * qy0) * worldToUv, 0.0f, 1.0f));
                const float qx1 = (p1.x - center.x) / radius;
                const float qy1 = (p1.y - center.y) / radius;
                const ImVec2 uv1(std::clamp(mapU + (cosHeading * qx1 + sinHeading * qy1) * worldToUv, 0.0f, 1.0f),
                                 std::clamp(mapV + (-sinHeading * qx1 + cosHeading * qy1) * worldToUv, 0.0f, 1.0f));
                const float qx2 = (p2.x - center.x) / radius;
                const float qy2 = (p2.y - center.y) / radius;
                const ImVec2 uv2(std::clamp(mapU + (cosHeading * qx2 + sinHeading * qy2) * worldToUv, 0.0f, 1.0f),
                                 std::clamp(mapV + (-sinHeading * qx2 + cosHeading * qy2) * worldToUv, 0.0f, 1.0f));
                const float qx3 = (p3.x - center.x) / radius;
                const float qy3 = (p3.y - center.y) / radius;
                const ImVec2 uv3(std::clamp(mapU + (cosHeading * qx3 + sinHeading * qy3) * worldToUv, 0.0f, 1.0f),
                                 std::clamp(mapV + (-sinHeading * qx3 + cosHeading * qy3) * worldToUv, 0.0f, 1.0f));

                draw->AddImageQuad(texture, p0, p1, p2, p3, uv0, uv1, uv2, uv3, IM_COL32_WHITE);
            }
        }
    }

    void DrawServerBlips(ImDrawList* draw, const ImVec2& center, float radius, float playerX, float playerY, float heading, float worldRange)
    {
        const float sinHeading = std::sin(heading);
        const float cosHeading = std::cos(heading);
        const float hudScale = radius / 146.0f;
        const float texturedSize = 17.0f * hudScale;
        const float edge = radius - texturedSize * 0.5f - 4.0f * hudScale;
        const float visibilityRangeSquared = g_miniMapBlipVisibilityRange * g_miniMapBlipVisibilityRange;

        for (const ServerBlip& blip : g_serverBlips)
        {
            if (!blip.active) continue;

            const float dx = blip.x - playerX;
            const float dy = blip.y - playerY;
            if (dx * dx + dy * dy > visibilityRangeSquared) continue;
            float screenX = (cosHeading * dx + sinHeading * dy) * radius / worldRange;
            float screenY = (sinHeading * dx - cosHeading * dy) * radius / worldRange;
            const float distance = std::sqrt(screenX * screenX + screenY * screenY);
            if (distance > edge && distance > 0.001f)
            {
                const float factor = edge / distance;
                screenX *= factor;
                screenY *= factor;
            }

            const ImVec2 pos(center.x + screenX, center.y + screenY);
            const float fallbackRadius = (blip.type == 0 ? 4.5f : 6.0f) * hudScale;
            DrawServerBlip(draw, blip, pos, texturedSize, fallbackRadius);
        }

        for (const LocalMapBlip& localBlip : kLocalMapBlips)
        {
            const ServerBlip& blip = localBlip.blip;
            const float dx = blip.x - playerX;
            const float dy = blip.y - playerY;
            if (dx * dx + dy * dy > visibilityRangeSquared) continue;
            float screenX = (cosHeading * dx + sinHeading * dy) * radius / worldRange;
            float screenY = (sinHeading * dx - cosHeading * dy) * radius / worldRange;
            const float distance = std::sqrt(screenX * screenX + screenY * screenY);
            if (distance > edge && distance > 0.001f)
            {
                const float factor = edge / distance;
                screenX *= factor;
                screenY *= factor;
            }
            DrawServerBlip(draw, blip, ImVec2(center.x + screenX, center.y + screenY),
                           texturedSize, 6.0f * hudScale);
        }
    }

    bool ClipLineToCircle(const ImVec2& center, float radius, ImVec2& start, ImVec2& end)
    {
        const float ax = start.x - center.x;
        const float ay = start.y - center.y;
        const float dx = end.x - start.x;
        const float dy = end.y - start.y;
        const float a = dx * dx + dy * dy;
        if (a < 0.0001f)
            return ax * ax + ay * ay <= radius * radius;

        const float b = 2.0f * (ax * dx + ay * dy);
        const float c = ax * ax + ay * ay - radius * radius;
        const float discriminant = b * b - 4.0f * a * c;
        if (discriminant < 0.0f) return false;

        const float root = std::sqrt(discriminant);
        const float enter = (-b - root) / (2.0f * a);
        const float exit = (-b + root) / (2.0f * a);
        const float from = std::max(0.0f, std::min(enter, exit));
        const float to = std::min(1.0f, std::max(enter, exit));
        if (from > to) return false;

        const ImVec2 originalStart = start;
        start = ImVec2(originalStart.x + dx * from, originalStart.y + dy * from);
        end = ImVec2(originalStart.x + dx * to, originalStart.y + dy * to);
        return true;
    }

    bool DrawRoadRouteOnMiniMap(ImDrawList* draw, const ImVec2& center, float radius,
                                float playerX, float playerY, float heading, float worldRange)
    {
        if (g_routeCalculating || g_roadRoute.size() < 2) return false;

        const float sinHeading = std::sin(heading);
        const float cosHeading = std::cos(heading);
        const auto worldToRadar = [&](float worldX, float worldY) {
            const float dx = worldX - playerX;
            const float dy = worldY - playerY;
            return ImVec2(center.x + (cosHeading * dx + sinHeading * dy) * radius / worldRange,
                          center.y + (sinHeading * dx - cosHeading * dy) * radius / worldRange);
        };

        const size_t lastSegment = g_roadRoute.size() - 2;
        const size_t firstSegment = std::min(g_routeProgressSegment, lastSegment);
        // Otimizacao: reduzido de 160 para 80 segmentos visiveis no minimapa.
        const size_t finalSegment = std::min(lastSegment, firstSegment + 80);
        bool drewAnySegment = false;
        for (size_t index = firstSegment; index <= finalSegment; ++index)
        {
            // A rota nunca é ligada diretamente à posição livre do jogador.
            // O trecho só começa no nó viário remanescente, igual ao mapa grande.
            const WorldPoint& startPoint = g_roadRoute[index];
            const WorldPoint& endPoint = g_roadRoute[index + 1];
            ImVec2 segmentStart = worldToRadar(startPoint.x, startPoint.y);
            ImVec2 segmentEnd = worldToRadar(endPoint.x, endPoint.y);
            if (!ClipLineToCircle(center, radius - 10.0f, segmentStart, segmentEnd)) continue;

            draw->AddLine(segmentStart, segmentEnd, IM_COL32(0, 0, 0, 175), 5.2f);
            draw->AddLine(segmentStart, segmentEnd, IM_COL32(177, 93, 255, 250), 2.8f);
            drewAnySegment = true;
        }
        return drewAnySegment;
    }

    void DrawDestinationOnMiniMap(ImDrawList* draw, const ImVec2& center, float radius,
                                  float playerX, float playerY, float heading, float worldRange)
    {
        if (!g_destinationActive || MaybeClearArrivedDestination(playerX, playerY)) return;

        const float sinHeading = std::sin(heading);
        const float cosHeading = std::cos(heading);
        const float edge = radius - 12.0f;
        const auto worldToRadarEdge = [&](float worldX, float worldY) {
            const float dx = worldX - playerX;
            const float dy = worldY - playerY;
            float screenX = (cosHeading * dx + sinHeading * dy) * radius / worldRange;
            float screenY = (sinHeading * dx - cosHeading * dy) * radius / worldRange;
            const float distance = std::sqrt(screenX * screenX + screenY * screenY);
            if (distance > edge && distance > 0.001f)
            {
                const float factor = edge / distance;
                screenX *= factor;
                screenY *= factor;
            }
            return ImVec2(center.x + screenX, center.y + screenY);
        };

        const float targetX = g_destinationUv.x * kWorldSize - kWorldHalf;
        const float targetY = kWorldHalf - g_destinationUv.y * kWorldSize;
        // O destino tem o mesmo marcador amarelo do mapa grande, inclusive quando
        // estiver fora do círculo: nesse caso ele fica corretamente preso à borda.
        const ImVec2 destination = worldToRadarEdge(targetX, targetY);
        DrawDestinationMarker(draw, destination, 20.0f);

        MaybeRecalculateRoadRoute(playerX, playerY);
        // Atualiza o progresso em todo frame do minimapa. Antes ele só era atualizado
        // ao abrir o mapa grande, por isso trechos já percorridos ficavam visíveis.
        if (g_roadRoute.size() >= 2)
            GetMiniMapGuidanceTarget(playerX, playerY, targetX, targetY);
        const bool drewRoadRoute = DrawRoadRouteOnMiniMap(draw, center, radius, playerX, playerY, heading, worldRange);
        // Antes de o A* terminar não há linha alternativa: evita apresentar uma
        // direção reta que não corresponde à malha de ruas.
        if (!drewRoadRoute) return;

    }

    void CenterLargeMapOnPlayer()
    {
        if (!pGame) return;
        CPlayerPed* playerPed = pGame->FindPlayerPed();
        bool inVehicle = false;
        CPlaceable* subject = GetRadarSubject(playerPed, inVehicle);
        if (!subject) return;

        const CVector& position = subject->GetPosition();
        g_largeMapCenterUv.x = std::clamp((position.x + kWorldHalf) / kWorldSize, 0.0f, 1.0f);
        g_largeMapCenterUv.y = std::clamp((kWorldHalf - position.y) / kWorldSize, 0.0f, 1.0f);
    }

    float GetHudScale(const ImVec2& display)
    {
        float scale = display.x / 1920.0f;
        const float verticalScale = display.y / 1080.0f;
        if (verticalScale < scale) scale = verticalScale;
        return scale > 0.0f ? scale : 1.0f;
    }

    float GetLargeMapLayerSize(const ImVec2& display)
    {
        // Mantém a cartografia 1:1. A área base é grande o suficiente para o
        // deslocamento horizontal em telas largas, mas não deforma o mapa.
        return std::max(display.x, display.y) * 0.75f * g_largeMapZoom;
    }

    void GetLargeMapSpans(const ImVec2& display, float& halfSpanX, float& halfSpanY)
    {
        const float layerSize = GetLargeMapLayerSize(display);
        halfSpanX = std::min(0.5f, display.x / (2.0f * layerSize));
        halfSpanY = std::min(0.5f, display.y / (2.0f * layerSize));
    }

    void ClampLargeMapCenter(float halfSpanX, float halfSpanY)
    {
        // Quando o mapa estiver menor que a tela, não o força de volta ao centro.
        // Isso preserva o deslocamento livre pedido pelo usuário e deixa o fundo
        // transparente aparecer naturalmente nas bordas.
        const float extra = 0.18f;
        const float minX = halfSpanX >= 0.5f ? -extra : halfSpanX;
        const float maxX = halfSpanX >= 0.5f ? 1.0f + extra : 1.0f - halfSpanX;
        const float minY = halfSpanY >= 0.5f ? -extra : halfSpanY;
        const float maxY = halfSpanY >= 0.5f ? 1.0f + extra : 1.0f - halfSpanY;
        g_largeMapCenterUv.x = std::clamp(g_largeMapCenterUv.x, minX, maxX);
        g_largeMapCenterUv.y = std::clamp(g_largeMapCenterUv.y, minY, maxY);
    }

    bool GetLargeMapPinchDistance(float& distance)
    {
        int first = -1;
        int second = -1;
        for (int index = 0; index < kLargeMapTouchCount; ++index)
        {
            if (!g_largeMapTouchDown[index]) continue;
            if (first < 0) first = index;
            else
            {
                second = index;
                break;
            }
        }

        if (first < 0 || second < 0) return false;
        const float dx = g_largeMapTouchPosition[first].x - g_largeMapTouchPosition[second].x;
        const float dy = g_largeMapTouchPosition[first].y - g_largeMapTouchPosition[second].y;
        distance = std::sqrt(dx * dx + dy * dy);
        return distance > 3.0f;
    }

    void ResetLargeMapTouches()
    {
        g_largeMapTouchDown.fill(false);
        g_largeMapTouchMoved.fill(false);
        g_largeMapLegendTouchIndex.fill(-1);
        g_largeMapLegendTouchAction.fill(kLegendTouchNone);
        g_largeMapDragging = false;
        g_largeMapPinching = false;
        g_largeMapPinchDistance = 0.0f;
    }

    void DrawLargeMap(ImDrawList* draw, const ImVec2& display)
    {
        const float scale = GetHudScale(display);
        const float mapSize = GetLargeMapLayerSize(display);
        float halfSpanX = 0.0f;
        float halfSpanY = 0.0f;
        GetLargeMapSpans(display, halfSpanX, halfSpanY);
        ClampLargeMapCenter(halfSpanX, halfSpanY);

        // O mapa inteiro permanece em uma única camada 1:1. O ponto central em UV
        // determina a posição da imagem na tela, em vez de recortar uma janela UV
        // dentro de um quadrado fixo.
        const ImVec2 mapStart(display.x * 0.5f - g_largeMapCenterUv.x * mapSize,
                              display.y * 0.5f - g_largeMapCenterUv.y * mapSize);
        const ImVec2 mapEnd(mapStart.x + mapSize, mapStart.y + mapSize);
        draw->AddImage(reinterpret_cast<ImTextureID>(g_mapTexture->raster), mapStart, mapEnd,
                       ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32_WHITE);

        const auto mapToScreen = [&](float uvX, float uvY) {
            return ImVec2(mapStart.x + uvX * mapSize, mapStart.y + uvY * mapSize);
        };
        const auto visible = [&](const ImVec2& point) {
            const float edge = 20.0f * scale;
            return point.x >= -edge && point.x <= display.x + edge &&
                   point.y >= -edge && point.y <= display.y + edge;
        };

        for (const ServerBlip& blip : g_serverBlips)
        {
            if (!blip.active) continue;
            const ImVec2 pos = mapToScreen((blip.x + kWorldHalf) / kWorldSize,
                                            (kWorldHalf - blip.y) / kWorldSize);
            if (!visible(pos)) continue;
            const float texturedSize = 21.0f * scale;
                        const float fallbackRadius = (blip.type == 0 ? 5.5f : 7.0f) * scale;
            DrawServerBlip(draw, blip, pos, texturedSize, fallbackRadius);
        }

        for (const LocalMapBlip& localBlip : kLocalMapBlips)
        {
            const ServerBlip& blip = localBlip.blip;
            const ImVec2 position = mapToScreen((blip.x + kWorldHalf) / kWorldSize,
                                                 (kWorldHalf - blip.y) / kWorldSize);
            if (visible(position))
                DrawServerBlip(draw, blip, position, 21.0f * scale, 7.0f * scale);
        }

        if (pGame)

        {
            CPlayerPed* playerPed = pGame->FindPlayerPed();
            bool inVehicle = false;
            CPlaceable* subject = GetRadarSubject(playerPed, inVehicle);
            if (subject)
            {
                const CVector& position = subject->GetPosition();
                const ImVec2 center = mapToScreen((position.x + kWorldHalf) / kWorldSize,
                                                   (kWorldHalf - position.y) / kWorldSize);

                if (g_destinationActive && !MaybeClearArrivedDestination(position.x, position.y))
                {
                    // Usa as mesmas decisões de chegada e desvio do minimapa
                    // antes de desenhar qualquer segmento no mapa grande.
                    MaybeRecalculateRoadRoute(position.x, position.y);
                    const ImVec2 destination = mapToScreen(g_destinationUv.x, g_destinationUv.y);
                    // Só é desenhada a polilinha confirmada da malha de ruas.
                    // Enquanto o A* está em execução não se exibe linha reta.
                    if (!g_routeCalculating && g_roadRoute.size() >= 2)
                    {
                        // Atualiza a mesma progressão usada no minimapa mesmo quando
                        // o mapa grande estiver aberto. O retorno em si não é usado
                        // aqui; ele apenas descarta os segmentos já percorridos.
                        const float targetX = g_destinationUv.x * kWorldSize - kWorldHalf;
                        const float targetY = kWorldHalf - g_destinationUv.y * kWorldSize;
                        GetMiniMapGuidanceTarget(position.x, position.y, targetX, targetY);

                        static std::vector<ImVec2> routePoints;
                        routePoints.clear();
                        const size_t routeStart = std::min(g_routeProgressSegment, g_roadRoute.size() - 1);
                        routePoints.reserve(g_roadRoute.size() - routeStart);
                        // Não inserir `center` aqui: ele representa a posição livre
                        // do jogador e criava a diagonal reta até a rota gravada.
                        for (size_t index = routeStart; index < g_roadRoute.size(); ++index)
                        {
                            const WorldPoint& point = g_roadRoute[index];
                            routePoints.push_back(mapToScreen((point.x + kWorldHalf) / kWorldSize,
                                                              (kWorldHalf - point.y) / kWorldSize));
                        }

                        if (routePoints.size() >= 2)
                        {
                            draw->AddPolyline(routePoints.data(), static_cast<int>(routePoints.size()),
                                              IM_COL32(0, 0, 0, 175), false, 7.0f * scale);
                            draw->AddPolyline(routePoints.data(), static_cast<int>(routePoints.size()),
                                              IM_COL32(177, 93, 255, 245), false, 3.6f * scale);
                        }
                    }
                    if (visible(destination))
                    {
                        DrawDestinationMarker(draw, destination, 28.0f * scale);
                    }
                }

                if (visible(center))
                {
                    const float size = 11.0f * scale;
                    // O mapa grande é norte para cima e sua coordenada vertical é
                    // invertida em relação ao mundo GTA. Espelhar o heading no eixo
                    // horizontal mantém a seta igual à orientação percebida no minimapa.
                    const float angle = -subject->GetHeading() - kPi * 0.5f;
                    const ImVec2 forward(std::cos(angle) * size, std::sin(angle) * size);
                    const ImVec2 side(-std::sin(angle) * size * 0.64f, std::cos(angle) * size * 0.64f);
                    const ImVec2 back(-forward.x * 0.58f, -forward.y * 0.58f);
                    draw->AddTriangleFilled(ImVec2(center.x + forward.x, center.y + forward.y),
                                            ImVec2(center.x + back.x + side.x, center.y + back.y + side.y),
                                            ImVec2(center.x + back.x - side.x, center.y + back.y - side.y),
                                            IM_COL32(245, 247, 250, 255));
                    draw->AddTriangle(ImVec2(center.x + forward.x, center.y + forward.y),
                                      ImVec2(center.x + back.x + side.x, center.y + back.y + side.y),
                                      ImVec2(center.x + back.x - side.x, center.y + back.y - side.y),
                                      IM_COL32(14, 17, 22, 255), 1.5f * scale);
                }
            }
        }

        // Legenda em estilo FiveM: nomes apenas dos pontos permanentes cadastrados
        // no cliente. Os blips dinâmicos do servidor não possuem texto no protocolo
        // atual, portanto continuam como ícones sem inventar nomes incorretos.
        DrawLargeMapLegend(draw, display, scale);

        // Botão maior e destacado, deslocado para a direita para não encostar na
        // borda da tela. Não possui janela: é desenhado diretamente sobre o mapa.
        const ImVec2 closeStart(58.0f * scale, display.y - 88.0f * scale);
        const ImVec2 closeEnd(closeStart.x + 158.0f * scale, closeStart.y + 56.0f * scale);
        draw->AddRectFilled(closeStart, closeEnd, IM_COL32(16, 18, 24, 222), 12.0f * scale);
        const char* closeLabel = "Fechar";
        const ImVec2 closeTextSize = ImGui::GetFont()->CalcTextSizeA(21.0f * scale, 2000.0f * scale, 0.0f, closeLabel);
        const ImVec2 closeText(closeStart.x + (closeEnd.x - closeStart.x - closeTextSize.x) * 0.5f,
                               closeStart.y + (closeEnd.y - closeStart.y - closeTextSize.y) * 0.5f - 1.0f * scale);
        draw->AddText(ImGui::GetFont(), 21.0f * scale, ImVec2(closeText.x + 1.5f * scale, closeText.y + 1.5f * scale),
                      IM_COL32(0, 0, 0, 190), closeLabel);
        draw->AddText(ImGui::GetFont(), 21.0f * scale, closeText, IM_COL32(250, 251, 252, 255), closeLabel);
    }
}

namespace NexionMiniMap
{
    void SetServerBlip(uint8_t id, float x, float y, float z, uint8_t type, uint32_t color, uint8_t style)
    {
        if (id >= g_serverBlips.size()) return;
        g_serverBlips[id] = { true, x, y, z, type, color, style };
    }

    void RemoveServerBlip(uint8_t id)
    {
        if (id >= g_serverBlips.size()) return;
        g_serverBlips[id] = {};
    }

    void ClearServerBlips()
    {
        g_serverBlips.fill({});
    }

    float GetMiniMapBlipVisibilityRange()
    {
        return g_miniMapBlipVisibilityRange;
    }

    void SetMiniMapBlipVisibilityRange(float range)
    {
        g_miniMapBlipVisibilityRange = std::clamp(range, 200.0f, 1200.0f);
    }

    bool ShouldReplaceNativeRadar()
    {
        // O hook só consulta esta função depois do spawn, quando o HUD manual está
        // ativo. Não manter o radar nativo como contingência evita que ele volte a
        // aparecer enquanto o asset do minimapa é carregado no frame da interface.
        return true;
    }

    void OpenLargeMap()
    {
        SetLargeMapOpenState(true);
        ResetLargeMapTouches();
        // Sempre abre no centro geométrico da cartografia com o zoom inicial leve.
        g_largeMapZoom = 0.45f;
        g_largeMapCenterUv = ImVec2(0.5f, 0.5f);
    }

    bool IsLargeMapOpen()
    {
        return g_largeMapOpen;
    }

    void Draw(ImDrawList* draw, const ImVec2& displaySize, float scale)
    {
        g_lastDisplaySize = displaySize;
        if (!draw || !pGame) return;

        // A logo Android do HUD so aparece depois que o jogador spawnou (o Java
        // a cria oculta). O radar so desenha com o jogador ativo, entao este e
        // o momento certo de libera-la — uma unica vez, sem chamar o Java a
        // cada frame.
        static bool s_hudLogoShown = false;
        if (!s_hudLogoShown)
        {
            s_hudLogoShown = true;
            if (pJavaWrapper) pJavaWrapper->SetNexionHudLogoVisible(true);
        }

        EnsureRoadGraphLoad();
        if (!EnsureMapTexture()) return;
        EnsureDestinationMarkerTexture();

        if (g_largeMapOpen)
        {
            DrawLargeMap(draw, displaySize);
            return;
        }

        CPlayerPed* playerPed = pGame->FindPlayerPed();
        bool inVehicle = false;
        CPlaceable* subject = GetRadarSubject(playerPed, inVehicle);
        if (!subject)
        {
            g_miniMapMotionInitialized = false;
            return;
        }

        const CVector& position = subject->GetPosition();
        const float targetHeading = GetMiniMapHeading(subject);
        float heading = targetHeading;
        float worldRange = inVehicle ? kVehicleRange : kOnFootRange;
        GetSmoothedMiniMapMotion(targetHeading, inVehicle, heading, worldRange);
        const float mapU = std::clamp((position.x + kWorldHalf) / kWorldSize, 0.0f, 1.0f);
        const float mapV = std::clamp((kWorldHalf - position.y) / kWorldSize, 0.0f, 1.0f);

        const float radius = 146.0f * scale;
        const float margin = 26.0f * scale;
        const ImVec2 center(margin + radius, margin + radius);

        DrawMapTexture(draw, center, radius, mapU, mapV, heading, worldRange);
        DrawServerBlips(draw, center, radius, position.x, position.y, heading, worldRange);
        DrawDestinationOnMiniMap(draw, center, radius, position.x, position.y, heading, worldRange);

        const float arrow = 11.0f * scale;
        // A cartografia gira pelo heading do radar (câmera quando parado e
        // entidade quando em movimento). A seta usa o heading real da entidade
        // dentro dessa mesma transformação: andando ela continua apontando para
        // cima; parado, ela gira junto com a cartografia em vez de ficar presa à
        // tela enquanto a câmera movimenta o mapa.
        const float arrowRotation = heading - subject->GetHeading();
        const float arrowSin = std::sin(arrowRotation);
        const float arrowCos = std::cos(arrowRotation);
        const auto rotateArrowPoint = [&](float x, float y)
        {
            return ImVec2(center.x + arrowCos * x - arrowSin * y,
                          center.y + arrowSin * x + arrowCos * y);
        };
        const ImVec2 arrowTop = rotateArrowPoint(0.0f, -arrow);
        const ImVec2 arrowLeft = rotateArrowPoint(-arrow * 0.72f, arrow * 0.72f);
        const ImVec2 arrowRight = rotateArrowPoint(arrow * 0.72f, arrow * 0.72f);
        draw->AddTriangleFilled(arrowTop, arrowLeft, arrowRight, IM_COL32(245, 247, 250, 255));
        draw->AddTriangle(arrowTop, arrowLeft, arrowRight, IM_COL32(17, 20, 25, 255), 1.8f * scale);

        // Borda simples, sem área preenchida atrás da cartografia.
        draw->AddCircle(center, radius + 2.0f * scale, IM_COL32(245, 247, 250, 238), 64, 1.8f * scale);

        const char* north = "N";
        draw->AddText(ImGui::GetFont(), 12.0f * scale,
                      ImVec2(center.x - 4.0f * scale, center.y - radius - 18.0f * scale),
                      IM_COL32(250, 250, 252, 220), north);

    }

    bool SetDestinationFromLargeMapTouch(const ImVec2& touch)
    {
        const float mapSize = GetLargeMapLayerSize(g_lastDisplaySize);
        if (mapSize <= 0.0f) return false;

        const ImVec2 mapStart(g_lastDisplaySize.x * 0.5f - g_largeMapCenterUv.x * mapSize,
                              g_lastDisplaySize.y * 0.5f - g_largeMapCenterUv.y * mapSize);
        const float u = (touch.x - mapStart.x) / mapSize;
        const float v = (touch.y - mapStart.y) / mapSize;
        if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) return false;

        // Um segundo toque simples cancela o destino, como solicitado.
        if (g_destinationActive)
        {
            ClearActiveDestination();
            return true;
        }

        // Ainda não há grafo carregado: não cria marcador provisório nem uma
        // linha reta. O próximo toque, após a carga terminar, será validado.
        if (!g_roadGraph) return false;

        const float requestedX = u * kWorldSize - kWorldHalf;
        const float requestedY = kWorldHalf - v * kWorldSize;
        int nearestNodeId = -1;
        float nearestDistanceSquared = 0.0f;
        const RoadNode* nearestRoad = FindNearestRoadNode(*g_roadGraph, requestedX, requestedY,
                                                           nearestNodeId, nearestDistanceSquared);
        const float maximumDistanceSquared = kMaxDestinationRoadDistance * kMaxDestinationRoadDistance;
        if (!nearestRoad || nearestDistanceSquared > maximumDistanceSquared)
        {
            FLog("NexionMiniMap: destino ignorado, fora da malha de ruas");
            return false;
        }

        // O marcador e o waypoint são encaixados no próprio nó viário. A ponta da
        // rota, portanto, nunca permanece em água ou distante da rua encontrada.
        const float worldX = nearestRoad->x;
        const float worldY = nearestRoad->y;
        g_destinationActive = true;
        g_destinationUv = ImVec2((worldX + kWorldHalf) / kWorldSize,
                                  (kWorldHalf - worldY) / kWorldSize);
        g_roadRoute.clear();
        g_routeProgressSegment = 0;
        g_routeCalculating = false;
        NexionMapSendWaypoint(worldX, worldY, 0.0f);
        StartRoadRouteCalculationForDestination();
        return true;
    }

    bool HandleTouch(int type, int pointerIndex, int x, int y)
    {
        if (g_lastDisplaySize.x <= 0.0f || g_lastDisplaySize.y <= 0.0f)
            return false;

        const int pointer = std::clamp(pointerIndex, 0, kLargeMapTouchCount - 1);
        const ImVec2 touch(static_cast<float>(x), static_cast<float>(y));
        const float scale = GetHudScale(g_lastDisplaySize);

        if (!g_largeMapOpen)
        {
            const float radius = 146.0f * scale;
            const float margin = 26.0f * scale;
            const ImVec2 center(margin + radius, margin + radius);
            const float dx = touch.x - center.x;
            const float dy = touch.y - center.y;
            const bool insideMiniMap = dx * dx + dy * dy <= (radius + 10.0f * scale) * (radius + 10.0f * scale);

            // O GTA mantém o botão invisível do radar antigo em uma área
            // retangular maior que o círculo desenhado. Sem esta guarda, tocar
            // em um dos cantos vazios abre o mapa original. Todo o retângulo do
            // radar Nexion é consumido; somente o círculo central abre o mapa
            // grande Nexion, enquanto os cantos não executam ação alguma.
            const float guardPadding = 34.0f * scale;
            const ImVec2 guardMin(std::max(0.0f, margin - guardPadding),
                                  std::max(0.0f, margin - guardPadding));
            const ImVec2 guardMax(center.x + radius + guardPadding,
                                  center.y + radius + guardPadding);
            const bool insideRadarTouchGuard = touch.x >= guardMin.x && touch.x <= guardMax.x &&
                                               touch.y >= guardMin.y && touch.y <= guardMax.y;

            if (type == 2 && insideRadarTouchGuard)
            {
                g_miniMapTouchActive = true;
                if (insideMiniMap)
                {
                    SetLargeMapOpenState(true);
                    ResetLargeMapTouches();
                    g_largeMapTouchDown[pointer] = true;
                    g_largeMapTouchPosition[pointer] = touch;
                    g_largeMapTouchStart[pointer] = touch;
                    g_largeMapTouchMoved[pointer] = false;
                    g_ignoreMapOpenTouchRelease = true;
                    // A abertura usa o centro geométrico da cartografia no centro
                    // visual da tela. Depois disso, o arrasto e o zoom permanecem livres.
                    g_largeMapZoom = 0.45f;
                    g_largeMapCenterUv = ImVec2(0.5f, 0.5f);
                }
                return true;
            }

            if (g_miniMapTouchActive)
            {
                if (type == 1) g_miniMapTouchActive = false;
                return true;
            }
            return false;
        }

        const float mapWidth = GetLargeMapLayerSize(g_lastDisplaySize);
        const float mapHeight = mapWidth;
        const ImVec2 closeStart(58.0f * scale, g_lastDisplaySize.y - 88.0f * scale);
        const ImVec2 closeEnd(closeStart.x + 158.0f * scale, closeStart.y + 56.0f * scale);
        const bool insideClose = touch.x >= closeStart.x && touch.x <= closeEnd.x && touch.y >= closeStart.y && touch.y <= closeEnd.y;

        if (type == 1)
        {
            const bool simpleTap = g_largeMapTouchDown[pointer] && !g_largeMapTouchMoved[pointer] && !g_largeMapPinching;
            const int legendItem = g_largeMapLegendTouchIndex[pointer];
            const int legendAction = g_largeMapLegendTouchAction[pointer];
            g_largeMapTouchDown[pointer] = false;
            g_largeMapLegendTouchIndex[pointer] = -1;
            g_largeMapLegendTouchAction[pointer] = kLegendTouchNone;

            if (g_ignoreMapOpenTouchRelease)
            {
                g_ignoreMapOpenTouchRelease = false;
                g_largeMapTouchMoved[pointer] = false;
                return true;
            }

            if (legendItem >= 0)
            {
                if (simpleTap)
                {
                    EnsureLocalMapBlipGroups();
                    if (legendItem < g_localMapBlipGroupCount)
                    {
                        LocalMapBlipGroup& group = g_localMapBlipGroups[static_cast<size_t>(legendItem)];
                        if (legendAction == kLegendTouchPrevious && group.selected > 0)
                            --group.selected;
                        else if (legendAction == kLegendTouchNext && group.selected + 1 < group.count)
                            ++group.selected;
                        const int localIndex = GetSelectedLocalMapBlipIndex(legendItem);
                        if (localIndex >= 0) CenterLargeMapOnLocalBlip(localIndex);
                    }
                }
                g_largeMapTouchMoved[pointer] = false;
                g_largeMapDragging = false;
                return true;
            }

            if (simpleTap && !insideClose)
                SetDestinationFromLargeMapTouch(touch);

            g_largeMapTouchMoved[pointer] = false;
            float remainingPinchDistance = 0.0f;
            if (!GetLargeMapPinchDistance(remainingPinchDistance))
            {
                g_largeMapPinching = false;
                g_largeMapPinchDistance = 0.0f;
                g_largeMapDragging = false;
            }
            return true;
        }

        g_largeMapTouchDown[pointer] = true;
        g_largeMapTouchPosition[pointer] = touch;
        if (type == 2)
        {
            g_largeMapTouchStart[pointer] = touch;
            g_largeMapTouchMoved[pointer] = false;
        }

        if (type == 2 && insideClose)
        {
            SetLargeMapOpenState(false);
            g_miniMapTouchActive = false;
            ResetLargeMapTouches();
            return true;
        }

        if (type == 2)
        {
            const int legendItem = FindLargeMapLegendItem(touch, g_lastDisplaySize, scale);
            if (legendItem >= 0)
            {
                g_largeMapLegendTouchIndex[pointer] = legendItem;
                g_largeMapLegendTouchAction[pointer] = GetLargeMapLegendTouchAction(legendItem, touch, g_lastDisplaySize, scale);
                g_largeMapDragging = false;
                return true;
            }
        }

        if (g_largeMapLegendTouchIndex[pointer] >= 0)
        {
            if (type == 3)
            {
                const ImVec2 total(touch.x - g_largeMapTouchStart[pointer].x,
                                   touch.y - g_largeMapTouchStart[pointer].y);
                const float tapThreshold = 10.0f * scale;
                if (total.x * total.x + total.y * total.y > tapThreshold * tapThreshold)
                    g_largeMapTouchMoved[pointer] = true;
            }
            return true;
        }

        float pinchDistance = 0.0f;
        if (GetLargeMapPinchDistance(pinchDistance))
        {
            if (!g_largeMapPinching)
            {
                g_largeMapPinching = true;
                g_largeMapPinchDistance = pinchDistance;
                g_largeMapDragging = false;
            }
            else if (type == 3 && g_largeMapPinchDistance > 0.0f)
            {
                const float factor = pinchDistance / g_largeMapPinchDistance;
                g_largeMapZoom = std::clamp(g_largeMapZoom * factor, 0.25f, 6.0f);
                g_largeMapPinchDistance = pinchDistance;
                float halfSpanX = 0.0f;
                float halfSpanY = 0.0f;
                GetLargeMapSpans(g_lastDisplaySize, halfSpanX, halfSpanY);
                ClampLargeMapCenter(halfSpanX, halfSpanY);
            }
            return true;
        }

        g_largeMapPinching = false;
        g_largeMapPinchDistance = 0.0f;
        if (type == 2)
        {
            // O arrasto começa em qualquer ponto da tela: a transparência visual não
            // limita mais a área de interação a um quadrado invisível.
            g_largeMapDragging = true;
            g_largeMapLastTouch = touch;
            return true;
        }

        if (type == 3)
        {
            if (!g_largeMapDragging)
            {
                g_largeMapDragging = true;
                g_largeMapLastTouch = touch;
                return true;
            }

            const ImVec2 delta(touch.x - g_largeMapLastTouch.x, touch.y - g_largeMapLastTouch.y);
            const ImVec2 total(touch.x - g_largeMapTouchStart[pointer].x, touch.y - g_largeMapTouchStart[pointer].y);
            const float tapThreshold = 10.0f * scale;
            if (total.x * total.x + total.y * total.y > tapThreshold * tapThreshold)
                g_largeMapTouchMoved[pointer] = true;
            float halfSpanX = 0.0f;
            float halfSpanY = 0.0f;
            GetLargeMapSpans(g_lastDisplaySize, halfSpanX, halfSpanY);
            g_largeMapCenterUv.x -= delta.x / mapWidth * (halfSpanX * 2.0f);
            g_largeMapCenterUv.y -= delta.y / mapHeight * (halfSpanY * 2.0f);
            g_largeMapLastTouch = touch;
            ClampLargeMapCenter(halfSpanX, halfSpanY);
            return true;
        }

        return true;
    }
}
