# Guia do Mapa, GPS e Blips — Nexion Online

> **Escopo:** este guia documenta o sistema nativo de minimapa, mapa grande, rota GPS e blips do cliente **Nexion Online** (`com.nexion.online`). Ele é renderizado em C++ com ImGui/RenderWare, no mesmo ciclo 2D do GTA. Não usa Flutter, WebView, `SurfaceView` ou uma tela Android para desenhar o mapa.[1] [2]

## 1. Visão geral

O sistema substitui o radar original do GTA por um radar circular persistente no canto superior esquerdo. Ao tocar nele, o jogador abre um mapa grande livre, com arrasto, zoom por pinça, destino GPS, rota por ruas reais e uma legenda lateral de locais.

| Componente | Função |
|---|---|
| **Minimapa circular** | Mostra a cartografia girando com a câmera, com a seta do jogador fixa para cima. |
| **Mapa grande** | Mostra a cartografia sem quadrado de recorte, com arrasto livre, pinça para zoom, marcador de destino e rota. |
| **GPS** | Calcula rota A* sobre a malha de ruas de `nodes.json`; não usa linha reta temporária. |
| **Blips locais** | Pontos permanentes definidos diretamente no cliente, como Prefeitura, lojas ou empregos. |
| **Blips do servidor** | Pontos atualizados por RPC da gamemode, com ícone escolhido pelo `type` recebido. |
| **Legenda lateral** | Lista os locais permanentes por nome. Itens repetidos são agrupados e alternados por setas. |

A substituição do radar, a captura dos toques e o desenho no HUD são feitos pelos hooks C++ e pelo HUD ImGui. O mapa grande mantém o modo limpo: os demais widgets do HUD e a logo Android são ocultados enquanto ele estiver aberto.[1] [2] [3]

---

## 2. Arquivos principais

| Arquivo | Responsabilidade | Quando editar |
|---|---|---|
| `app/src/main/cpp/samp/gui/nexion_minimap.cpp` | Toda a lógica e o desenho de mapa, GPS, blips, legenda e gestos. | Para configurar tamanhos, distâncias, nomes, posições e comportamento. |
| `app/src/main/cpp/samp/gui/nexion_minimap.h` | Interface pública usada pelos hooks/RPCs. | Só ao criar uma nova função pública do sistema. |
| `app/src/main/cpp/samp/gui/gui.cpp` | Chama `NexionMiniMap::Draw()` e oculta o restante do HUD no mapa grande. | Normalmente não é necessário para cadastrar locais. |
| `app/src/main/cpp/samp/game/hooks.cpp` | Oculta o radar nativo, encaminha toques e integra waypoint/RPC. | Para integrar uma nova origem de blips ou evento de toque. |
| `app/src/main/java/com/wardrumstudios/utils/WarMedia.java` | Copia os assets de mapa para o armazenamento do jogo, com marcadores de versão. | Para publicar versões novas de assets ou ícones. |
| `app/src/main/assets/nexion_radar/` | Assets incluídos no APK. | Para trocar mapa, marcador, grafo ou sprites de blip. |
| `app/src/main/cpp/samp/CMakeLists.txt` | Build da biblioteca nativa `samp`. | Não é preciso listar `nexion_minimap.cpp` manualmente: o projeto coleta fontes C/C++ recursivamente. |

> **Regra importante:** a source de trabalho é `nexion-rp-base-original-verified`. Não edite outra cópia por engano.

---

## 3. Fluxo de renderização e integração

A cada frame, o HUD chama `NexionMiniMap::Draw()`. Quando o mapa grande está fechado, essa função desenha o minimapa circular. Quando está aberto, desenha o mapa livre, a legenda lateral e o botão **Fechar**. O hook impede o desenho do radar original enquanto o HUD Nexion está ativo, evitando que dois mapas apareçam juntos.[1] [2] [3]

O fluxo é o seguinte:

```text
GTA / hook de renderização
        |
        +--> gui.cpp: DrawNexionHud()
                 |
                 +--> NexionMiniMap::Draw()
                         |
                         +--> minimapa circular OU mapa grande
                         +--> rota GPS, destino e blips

Toque Android
        |
        +--> hooks.cpp: AND_TouchEvent_hook()
                 |
                 +--> NexionMiniMap::HandleTouch()
                         |
                         +--> minimapa / mapa grande / legenda / rota
```

O hook de armazenamento define o diretório runtime do cliente como:

```text
/storage/emulated/0/Android/data/com.nexion.online/files/SAMP/nexion_radar/
```

Os assets são copiados do APK para esse diretório na inicialização da activity.[2] [4]

---

## 4. Assets do mapa

Os assets abaixo devem existir em `app/src/main/assets/nexion_radar/` antes de compilar o APK.

| Asset no APK | Formato e tamanho | Destino runtime | Uso |
|---|---:|---|---|
| `map_minimap.rgba` | RGBA bruto, `1024 × 1024`, 4 bytes por pixel | `SAMP/nexion_radar/map_minimap.rgba` | Cartografia do minimapa e do mapa grande. |
| `nodes.json` | JSON da malha viária | `SAMP/nexion_radar/nodes.json` | Grafo de ruas usado pelo A*. |
| `marker.rgba` | RGBA bruto, `43 × 43` | `SAMP/nexion_radar/marker.rgba` | Marcador amarelo do destino GPS. |
| `blips/blip_1.rgba` … `blips/blip_194.rgba` | RGBA bruto, `32 × 32`, exatamente 4096 bytes cada | `SAMP/nexion_radar/blips/` | Ícones de blip escolhidos pelo tipo. |

### 4.1 Por que os ícones usam RGBA

Os sprites são RGBA bruto porque o RenderWare pode criar a textura diretamente a partir dos pixels, preservando o alpha. Não é necessário decodificar PNG durante a renderização. O custo é que RGBA não tem compressão; por isso os ícones foram mantidos em `32 × 32` e só são carregados para a memória de vídeo quando o respectivo tipo é usado.[1] [4]

Para criar ou trocar um ícone, use esta regra:

```text
largura × altura × 4 bytes = tamanho esperado
32 × 32 × 4 = 4096 bytes
```

A ordem dos bytes precisa ser **R, G, B, A**. O arquivo deve se chamar exatamente `blip_<tipo>.rgba`, por exemplo:

```text
blip_3.rgba
blip_35.rgba
blip_194.rgba
```

### 4.2 Forçar cópia de assets novos

`WarMedia.java` usa arquivos `.ready` para não copiar o mesmo asset em toda inicialização. Se trocar um asset e precisar garantir que ele seja copiado novamente em celulares que já instalaram o APK, aumente o nome ou a versão do marcador correspondente.

| Asset alterado | Ponto a alterar em `WarMedia.java` |
|---|---|
| `map_minimap.rgba` | Altere `map_nexion_radar_v4.ready` e o valor de versão `4`. |
| `nodes.json` | Altere `map_nexion_gps_v1.ready` e o valor de versão `1`. |
| `marker.rgba` | Altere `map_nexion_destination_marker_v1.ready` e o valor de versão `1`. |
| Qualquer `blip_*.rgba` | Altere `map_nexion_blips_v1.ready` para uma versão nova, por exemplo `map_nexion_blips_v2.ready`. |

> Para testes locais, também é possível apagar o arquivo `.ready` correspondente no diretório runtime. Na próxima abertura, a activity copiará o asset de novo.

---

## 5. Conversão entre coordenadas do GTA e mapa

O mapa usa uma área de mundo de `-3000` até `+3000` em X e Y. A textura tem `1024 × 1024` pixels. A conversão usada internamente é:

```cpp
constexpr float kWorldHalf = 3000.0f;
constexpr float kWorldSize = 6000.0f;

float mapX = ((worldX + 3000.0f) / 6000.0f) * 1024.0f;
float mapY = ((3000.0f - worldY) / 6000.0f) * 1024.0f;
```

| Valor | Significado |
|---|---|
| `worldX`, `worldY`, `worldZ` | Coordenadas reais do GTA/SA-MP. |
| `mapX`, `mapY` | Posição equivalente dentro da textura do mapa. |
| `Z` | É mantido no blip e no GPS, mas a projeção visual do mapa usa X/Y. |

Ao cadastrar um local, use sempre coordenadas GTA **X, Y e Z**. Não converta para pixels manualmente; o código já faz isso.

---

## 6. Configurações principais do minimapa e mapa grande

As constantes abaixo ficam no início de `nexion_minimap.cpp`. Estes são os valores atuais documentados nesta versão do guia.[1]

| Constante | Valor atual | Efeito | Ajuste recomendado |
|---|---:|---|---|
| `kOnFootRange` | `600.0f` | Área do mundo mostrada a pé. | Menor aproxima; maior afasta. |
| `kVehicleRange` | `700.0f` | Área do mundo mostrada em veículo. | Deve ser maior que a pé se quiser ver um pouco mais da rua ao dirigir. |
| `kMiniMapBlipVisibilityRange` | `480.0f` | Distância máxima para um blip aparecer no minimapa. | Menor reduz poluição; maior mostra locais antes. |
| `kLegendFocusZoom` | `0.56f` | Zoom mínimo aplicado ao tocar um item na legenda. | Maior aproxima mais o mapa grande. |
| `kMaxDestinationRoadDistance` | `60.0f` | Distância máxima até uma via para aceitar destino GPS. | Não aumente demais, ou água/áreas vazias podem aceitar destino. |
| `kRouteDeviationDistance` | `35.0f` | Distância para considerar que o jogador saiu da rota. | Menor recalcula mais cedo; maior tolera atalhos. |
| `kRouteRecalculationCooldownMs` | `1200` ms | Intervalo mínimo entre recálculos. | Não reduza muito para evitar trabalho excessivo. |
| `kDestinationArrivalDistance` | `16.0f` | Raio de chegada que remove destino e rota. | Aumente se quiser concluir antes. |
| `g_largeMapZoom` | `0.45f` inicial | Escala inicial ao abrir o mapa grande. | Menor deixa o mapa mais afastado; maior aproxima. |

### 6.1 Suavização ao entrar ou sair de veículo

O minimapa usa interpolação exponencial para não trocar o alcance bruscamente. O heading é suavizado com resposta mais rápida e o alcance com resposta mais lenta. Não remova `GetSmoothedMiniMapMotion()` ao editar a lógica de zoom; ele é responsável por manter essa transição fluida.[1]

---

## 7. Adicionar locais permanentes no cliente

Os locais permanentes, que aparecem sem depender da gamemode, ficam na lista `kLocalMapBlips` dentro de `nexion_minimap.cpp`.[1]

A estrutura é:

```cpp
struct LocalMapBlip
{
    const char* name = "";
    ServerBlip blip{};
};
```

O cadastro atual serve como modelo:

```cpp
constexpr size_t kLocalMapBlipCount = 3;
const std::array<LocalMapBlip, kLocalMapBlipCount> kLocalMapBlips{{
    { "Prefeitura", { true, 1481.0f, -1769.0f, 13.5f, 3, 0xFFFFFFFF, 0 } },
    { "Teste",      { true, 1230.0f, -1380.0f, 13.5f, 35, 0xFFFFFFFF, 0 } },
    { "Teste",      { true, -1975.0f, 135.0f, 27.0f, 35, 0xFFFFFFFF, 0 } },
}};
```

A ordem de cada item é:

```cpp
{ "NOME", { ativo, X, Y, Z, tipoDoIcone, corRGBA, estilo } }
```

| Campo | Exemplo | Função |
|---|---|---|
| `NOME` | `"Prefeitura"` | Nome mostrado na legenda lateral. |
| `ativo` | `true` | Mantém o ponto ativo. |
| `X, Y, Z` | `1481.0f, -1769.0f, 13.5f` | Coordenadas reais do GTA. |
| `tipoDoIcone` | `3` | Busca `blip_3.rgba`. |
| `corRGBA` | `0xFFFFFFFF` | Mantém a cor original do sprite. |
| `estilo` | `0` | Campo reservado para estilo do blip. |

### 7.1 Exemplo: adicionar uma oficina

Se havia 3 locais e você quer acrescentar uma oficina, faça **duas alterações**: atualize a quantidade e adicione a linha.

```cpp
constexpr size_t kLocalMapBlipCount = 4;
const std::array<LocalMapBlip, kLocalMapBlipCount> kLocalMapBlips{{
    { "Prefeitura", { true, 1481.0f, -1769.0f, 13.5f, 3, 0xFFFFFFFF, 0 } },
    { "Teste",      { true, 1230.0f, -1380.0f, 13.5f, 35, 0xFFFFFFFF, 0 } },
    { "Teste",      { true, -1975.0f, 135.0f, 27.0f, 35, 0xFFFFFFFF, 0 } },
    { "Mecânica LS", { true, 1680.0f, -1830.0f, 13.5f, 42, 0xFFFFFFFF, 0 } },
}};
```

Depois compile e instale o APK. Caso o tipo `42` ainda não exista em `blips/`, o sistema desenhará um ponto colorido como fallback até o arquivo `blip_42.rgba` ser incluído.[1]

### 7.2 Várias posições com o mesmo nome

Locais com o mesmo nome **não duplicam linhas na legenda lateral**. Eles formam um grupo.

```cpp
{ "Aeroporto", { true, 1680.0f, -2330.0f, 13.5f, 12, 0xFFFFFFFF, 0 } },
{ "Aeroporto", { true, -1420.0f, -290.0f, 14.0f, 12, 0xFFFFFFFF, 0 } },
{ "Aeroporto", { true, -1230.0f, 55.0f, 14.0f, 12, 0xFFFFFFFF, 0 } },
```

A legenda exibirá uma única linha:

```text
Aeroporto   < 1/3 >   [ícone]
```

Ao tocar em `>`, a seleção passa para `2/3` e o mapa grande centraliza na segunda posição. A seta `<` volta para a posição anterior. O minimapa e o mapa grande continuam desenhando **todos** os pontos desse grupo.

> A área esquerda da legenda é “anterior” (`<`) e a direita é “próxima” (`>`). Não inverta somente o desenho sem preservar a ação de toque correspondente.

### 7.3 Remover um local

Remova a linha desejada de `kLocalMapBlips` e reduza `kLocalMapBlipCount` para a nova quantidade. Se deixar a quantidade antiga, o C++ não compilará corretamente.

---

## 8. Legenda lateral do mapa grande

A legenda aparece somente no mapa grande. Cada item tem nome, ícone e fundo escuro ajustado ao comprimento do texto. O minimapa não exibe nomes para continuar limpo.

| Interação | Resultado |
|---|---|
| Toque no nome ou no ícone | Centraliza no local escolhido e aplica o zoom mínimo de foco. |
| Toque em `<` | Seleciona a posição anterior de um grupo. |
| Toque em `>` | Seleciona a posição seguinte de um grupo. |
| Grupo com uma posição | Não mostra contador ou setas. |
| Grupo com N posições | Mostra `1/N`, `2/N`, …, `N/N`. |

O layout já mede a largura integral do texto antes de desenhar a faixa. Por isso, ao adicionar nomes maiores, o fundo aumenta até o espaço disponível da tela.[1]

---

## 9. Blips dinâmicos vindos do servidor

O sistema também possui cache para até 100 blips recebidos da gamemode. A interface pública é:[5]

```cpp
NexionMiniMap::SetServerBlip(id, x, y, z, type, color, style);
NexionMiniMap::RemoveServerBlip(id);
NexionMiniMap::ClearServerBlips();
```

| Parâmetro | Descrição |
|---|---|
| `id` | Slot do blip no cache; use valores entre `0` e `99`. |
| `x, y, z` | Coordenadas GTA enviadas pelo servidor. |
| `type` | Tipo visual; `1` a `194` procura o arquivo `blip_<type>.rgba`. |
| `color` | Cor RGBA aplicada ao sprite. `0xFFFFFFFF` mantém a aparência original. |
| `style` | Campo de estilo disponível para extensões futuras. |

Exemplo de integração em um handler de RPC:

```cpp
// Depois de validar os dados recebidos no RPC:
NexionMiniMap::SetServerBlip(
    blipId,
    positionX,
    positionY,
    positionZ,
    blipType,
    blipColor,
    blipStyle
);
```

Se a gamemode remover o ponto, envie:

```cpp
NexionMiniMap::RemoveServerBlip(blipId);
```

Na troca de servidor, logout ou reinício de dados, use:

```cpp
NexionMiniMap::ClearServerBlips();
```

Os blips dinâmicos aparecem no mapa grande. No minimapa, eles obedecem `kMiniMapBlipVisibilityRange`; logo, só aparecem quando o jogador está suficientemente próximo.[1] [5]

> **Importante:** a legenda lateral atual é destinada aos locais permanentes configurados em `kLocalMapBlips`. Um blip dinâmico do servidor não recebe nome automaticamente, porque o protocolo atual do cache contém posição, tipo, cor e estilo, mas não um texto de nome. Para listá-lo, seria necessário ampliar o protocolo e a estrutura de dados com um campo `name`.

---

## 10. GPS e rota por ruas

O GPS é calculado sobre `nodes.json`, uma malha de ruas. O carregamento do grafo ocorre de forma assíncrona após o spawn para não bloquear o HUD. Quando o jogador marca um destino no mapa grande, o sistema localiza o nó viário mais próximo, executa A* e desenha uma rota roxa.[1]

| Evento | Comportamento |
|---|---|
| Primeiro toque em uma rua no mapa grande | Marca o destino e solicita a rota. |
| Segundo toque enquanto há destino | Remove destino e rota. |
| Toque a mais de 60 unidades de uma rua | Não aceita o destino. |
| Jogador sai mais de 35 unidades da rota | Recalcula o caminho, respeitando 1,2 s de intervalo mínimo. |
| Jogador chega a 16 unidades do destino | Remove automaticamente rota e marcador. |
| A* ainda calculando | Nenhuma linha reta provisória é desenhada. |

O marcador de destino usa `marker.rgba`; a rota aparece nos dois mapas. O trecho já percorrido é removido progressivamente para que a rota restante seja mostrada.[1]

---

## 11. Interações do mapa grande

| Gesto ou ação | Resultado |
|---|---|
| Tocar no minimapa circular | Abre o mapa grande. |
| Arrastar com um dedo no mapa grande | Move o mapa livremente em qualquer direção. |
| Pinça com dois dedos | Aumenta ou diminui o zoom sem distorcer a cartografia. |
| Tocar em área livre do mapa | Marca/remove um destino GPS, quando não for arrasto. |
| Tocar em item da legenda | Foca o local selecionado e aplica zoom leve. |
| Tocar no botão **Fechar** | Fecha o mapa grande, restaura os widgets do HUD e a logo. |

A abertura do mapa grande não deve chamar `DisplayHUD(true)`. Essa chamada reativa o HUD original inteiro do GTA e cria conflitos visuais. O projeto mantém o HUD original desativado e desenha somente o HUD Nexion.[3]

---

## 12. Ajustes visuais úteis

Os números abaixo são os pontos mais comuns para ajuste em `nexion_minimap.cpp`.

| O que mudar | Onde procurar | Resultado |
|---|---|---|
| Tamanho do círculo | Raio do minimapa dentro do desenho circular | Aumenta/diminui o radar persistente. |
| Alcance a pé | `kOnFootRange` | Aproxima/afasta a cartografia enquanto a pé. |
| Alcance em veículo | `kVehicleRange` | Aproxima/afasta a cartografia dentro do carro. |
| Distância de blips no minimapa | `kMiniMapBlipVisibilityRange` | Faz ícones aparecerem mais cedo ou mais tarde. |
| Zoom ao clicar na legenda | `kLegendFocusZoom` | Aproxima mais ou menos o local selecionado. |
| Zoom inicial do mapa grande | `g_largeMapZoom` | Muda apenas a abertura padrão. |
| Tamanho de ícones | `texturedSize` nos desenhos de blip | Altera os sprites no minimapa e/ou mapa grande. |
| Legenda lateral | `DrawLargeMapLegend()` e `GetLargeMapLegendLayout()` | Ajusta altura, fonte, espaçamento, ícone, setas e contador. |

Faça uma alteração por vez e teste no servidor online. Assim fica simples identificar se o ajuste visual teve o efeito desejado.

---

## 13. Compilar e validar

1. Aplique o patch ou altere os arquivos necessários na source correta.
2. Confirme que novos assets estão dentro de `app/src/main/assets/nexion_radar/`.
3. Se trocou assets existentes, aumente o marcador correspondente em `WarMedia.java`.
4. Compile o APK no Android Studio/Gradle com NDK configurado para `armeabi-v7a` e `arm64-v8a`.
5. Instale por cima da versão anterior ou remova os marcadores `.ready` durante teste local.
6. Entre em um servidor online e valide o comportamento real de spawn, toques, RPCs e rotas.

Checklist de teste:

| Teste | Resultado esperado |
|---|---|
| Entrar no servidor | Sem travamento; minimapa aparece apenas após o jogador existir no mundo. |
| Andar a pé | Mapa acompanha posição e câmera; seta permanece fixa para cima. |
| Entrar em veículo | Alcance muda suavemente para o valor de veículo. |
| Mover a câmera parado | A cartografia gira acompanhando a câmera, sem girar a seta. |
| Abrir mapa grande | HUD e logo somem; aparecem mapa, legenda e botão Fechar. |
| Arrastar e pinçar | Movimento livre em X/Y e zoom suave, sem quadrado de recorte. |
| Clicar em legenda | Centraliza o local correto; grupos exibem contador e setas corretas. |
| Marcar destino em rua | Marcador e rota roxa aparecem após A*. |
| Marcar água/área sem rua | Destino é recusado. |
| Desviar da rota | Rota recalcula após o limite de desvio. |
| Aproximar de blip | Ícone aparece no minimapa dentro do alcance configurado. |

---

## 14. Diagnóstico rápido

| Sintoma | Causa provável | Correção |
|---|---|---|
| Mapa preto, branco ou ausente | Asset RGBA inválido, incompleto ou marcador desatualizado. | Confirme dimensões, bytes, nome do arquivo e aumente o marcador em `WarMedia.java`. |
| Ícone vira bolinha colorida | Sprite do tipo não existe ou não possui 4096 bytes. | Inclua `blip_<tipo>.rgba` em `assets/nexion_radar/blips/`. |
| Novo ícone não atualiza no celular | O `.ready` antigo impediu a cópia. | Mude o nome/versão de `map_nexion_blips_v*.ready` ou apague o marcador runtime. |
| Dois mapas aparecem | Radar nativo não foi suprimido pelo hook. | Confira `ShouldReplaceNativeRadar()` e o hook de `CHud::DrawRadar`. |
| Toque abre o mapa nativo | `HandleTouch()` não consumiu o evento antes do GTA. | Revise o encaminhamento em `AND_TouchEvent_hook()`. |
| Nome aparece cortado | Layout da legenda foi alterado sem medir o texto completo. | Preserve a medição via `CalcTextSizeA()` e o fundo adaptativo. |
| Setas fazem ação oposta | Geometria e ação de toque foram trocadas em conjunto. | Área esquerda deve ser anterior `<`; direita deve ser próxima `>`. |
| Rota fica reta | Grafo não foi carregado ou foi usada lógica provisória. | Confirme `nodes.json`, o carregamento assíncrono e a execução do A*. |
| HUD original volta a aparecer | Algum código chamou `DisplayHUD(true)`. | Remova essa chamada; mantenha o HUD GTA desativado. |
| Erro Java em `onDestroy()` | Método ficou com visibilidade menor que a classe pai. | Mantenha `public void onDestroy()`. |

---

## 15. Limites e boas práticas

| Regra | Motivo |
|---|---|
| Não usar Flutter/WebView para este mapa. | O sistema atual é nativo, leve e sincronizado com o pipeline GTA. |
| Não chamar `DisplayHUD(true)`. | Reativa elementos originais do GTA e quebra a substituição do HUD. |
| Manter `onDestroy()` como `public`. | A activity pai exige essa visibilidade. |
| Usar `m_forward`, não `m_at`. | `m_forward` é o vetor usado para orientação da entidade nesta source. |
| Não carregar todos os 194 sprites na abertura. | O carregamento sob demanda preserva memória de vídeo. |
| Aumentar versões `.ready` quando trocar assets. | Garante atualização em instalações já existentes. |
| Testar sempre online. | RPCs, spawn, rota e input dependem do fluxo real do cliente/servidor. |
| Entregar apenas arquivos modificados. | Evita substituir a source completa e reduz conflitos. |

---

## 16. Modelo de pedido para novos locais

Quando quiser adicionar locais, envie as informações desta forma:

```text
Nome: Mecânica LS
Ícone: 42
Posição 1: X, Y, Z
Posição 2: X, Y, Z   (opcional; use o mesmo nome para criar 1/2, 2/2)
Cor: padrão / outra cor (opcional)
```

Exemplo:

```text
Nome: Posto de gasolina
Ícone: 35
Posição 1: 1000.0, -940.0, 42.0
Posição 2: -1675.0, 410.0, 7.0
```

Com o mesmo nome, o mapa criará uma linha **Posto de gasolina** na legenda e permitirá alternar as duas posições com `< 1/2 >`.

---

## Referências

[1]: app/src/main/cpp/samp/gui/nexion_minimap.cpp "Implementação principal de minimapa, mapa grande, GPS, blips e legenda"
[2]: app/src/main/cpp/samp/game/hooks.cpp "Hooks de radar, renderização, toque, waypoint e armazenamento"
[3]: app/src/main/cpp/samp/gui/gui.cpp "Integração do minimapa ao HUD ImGui e modo de mapa limpo"
[4]: app/src/main/java/com/wardrumstudios/utils/WarMedia.java "Instalação versionada dos assets nativos no armazenamento do jogo"
[5]: app/src/main/cpp/samp/gui/nexion_minimap.h "Interface pública de blips, desenho e toque"
[6]: app/src/main/cpp/samp/CMakeLists.txt "Inclusão recursiva de fontes C/C++ no módulo samp"

**Autor:** Manus AI  
**Documento para:** Nexion Online  
**Estado documentado:** minimapa/GPS/blips com ajustes até o patch v11.
