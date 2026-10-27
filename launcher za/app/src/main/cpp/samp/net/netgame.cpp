#include "../main.h"
#include "../game/game.h"
#include <atomic>
#include "netgame.h"
#include "../gui/gui.h"
#include "../gui/nexion_minimap.h"
#include "../gui/CharacterCreator.h"

#include "../audiostream.h"

// voice
#include "../voice_new/MicroIcon.h"
#include "../voice_new/SpeakerList.h"
#include "../voice_new/Network.h"
#include "java/jniutil.h"

//#define AUTH_BS "39FB2DEEDB49ACFB8D4EECE6953D2507988CCCF4410"//main
#define AUTH_BS "E02262CF28BC542486C558D4BE9EFB716592AFAF8B"

extern UI* pUI;
extern CGame* pGame;
extern CAudioStream* pAudioStream;

//extern CVoice* pVoice;

int iNetModeNormalOnFootSendRate = 30;
int iNetModeNormalInCarSendRate = 30;
int iNetModeFiringSendRate = 30;
int iNetModeSendMultiplier = 2;

void RegisterRPCs(RakClientInterface *pRakClient);
void UnregisterRPCs(RakClientInterface *pRakClient);
void RegisterScriptRPCs(RakClientInterface *pRakClient);
void UnregisterScriptRPCs(RakClientInterface *pRakClient);

void InstallVehicleEngineLightPatches();

// 0.3.7
unsigned char GetPacketID(Packet *p)
{
	if (p == 0) return 255;

	if ((unsigned char)p->data[0] == ID_TIMESTAMP)
		return (unsigned char)p->data[sizeof(unsigned char) + sizeof(unsigned long)];
	else
		return (unsigned char)p->data[0];
}

extern CJavaWrapper *pJavaWrapper;

CNetGame::CNetGame(const char* szHostOrIp, int iPort, const char *szPlayerName, const char* szPass)
{
	FLog("CNetGame initializing..");

	// voice
	//Network::OnRaknetConnect(szHostOrIp, iPort);

	//MyLog2("Voice connect %s:%d", szHostOrIp, iPort);
	//MyLog2("Voice connect %s:%d", szHostOrIp, iPort);
	//MyLog2("Voice connect %s:%d", szHostOrIp, iPort);

	m_pNetSet = new NET_SETTINGS;
	memset(m_szHostName, 0, 256);
	memset(m_szHostOrIp, 0, 256);

	strcpy(m_szHostName, "SA-MP");
	strncpy(m_szHostOrIp, szHostOrIp, sizeof(m_szHostOrIp));
	m_iPort = iPort;

	m_pRakClient = RakNetworkFactory::GetRakClientInterface();
	InitializePools();

	GetPlayerPool()->SetLocalPlayerName(szPlayerName);

	RegisterRPCs(m_pRakClient);
	RegisterScriptRPCs(m_pRakClient);
	m_pRakClient->SetPassword(szPass);

	memset(m_dwMapIcon, 0, sizeof(m_dwMapIcon));

	pGame->EnableClock(false);
	pGame->EnableZoneNames(false);

	m_pNetSet->iDeathDropMoney = 0;
	m_pNetSet->iSpawnsAvailable = 0;
	m_pNetSet->bNameTagLOS = 0;
	m_pNetSet->byteHoldTime = true;
	m_pNetSet->bUseCJWalk = 0;
	m_pNetSet->bDisableInteriorEnterExits = 0;
	m_pNetSet->bZoneNames = false;
	m_pNetSet->bInstagib = false;
	m_pNetSet->fNameTagDrawDistance = 70.0f;
	m_pNetSet->bFriendlyFire = true;
	m_pNetSet->byteWorldTime_Hour = 12;
	m_pNetSet->byteWorldTime_Minute = 0;
	m_pNetSet->byteWeather = 1;
	m_pNetSet->fGravity = 0.008f;
	m_bNameTagStatus = true;

	m_dwLastConnectAttempt = GetTickCount();
	SetGameState(GAMESTATE_WAIT_CONNECT);
	m_bLanMode = false;

    if(pJavaWrapper)
        pJavaWrapper->HideLoadingScreen();

    const char* sampVer = SAMP_VERSION;
    if(pSettings)
        sampVer = pSettings->Get().szVersion;
}
// 0.3.7
CNetGame::~CNetGame()
{
	// voice
	//Network::OnRaknetDisconnect();

	m_pRakClient->Disconnect(0);
	UnregisterRPCs(m_pRakClient);
	UnregisterScriptRPCs(m_pRakClient);
	RakNetworkFactory::DestroyRakClientInterface(m_pRakClient);

	UninitializePools();

	if (m_pNetSet) {
		delete m_pNetSet;
		m_pNetSet = nullptr;
	}
}

void CNetGame::InitializePools()
{
	m_pPools = new NET_POOLS;
	m_pPools->pPlayerPool = new CPlayerPool();
	m_pPools->pVehiclePool = new CVehiclePool();
	m_pPools->pGangZonePool = new CGangZonePool();
	m_pPools->pPickupPool = new CPickupPool();
	m_pPools->pObjectPool = new CObjectPool();
	m_pPools->pTextLabelPool = new C3DTextLabelPool();
	m_pPools->pTextDrawPool = new CTextDrawPool();
	m_pPools->pActorPool = new CActorPool();
	m_pPools->pMenuPool = new CMenuPool();
	m_pPools->pPlayerBubblePool = new CPlayerBubblePool();
}

void CNetGame::UninitializePools()
{
	if (m_pPools->pPlayerPool) {
		delete m_pPools->pPlayerPool;
		m_pPools->pPlayerPool = nullptr;
	}

	if (m_pPools->pVehiclePool) {
		delete m_pPools->pVehiclePool;
		m_pPools->pVehiclePool = nullptr;
	}

	if (m_pPools->pGangZonePool) {
		delete m_pPools->pGangZonePool;
		m_pPools->pGangZonePool = nullptr;
	}

	if (m_pPools->pPickupPool) {
		delete m_pPools->pPickupPool;
		m_pPools->pPickupPool = nullptr;
	}

	if (m_pPools->pObjectPool) {
		delete m_pPools->pObjectPool;
		m_pPools->pObjectPool = nullptr;
	}

	if (m_pPools->pTextLabelPool) {
		delete m_pPools->pTextLabelPool;
		m_pPools->pTextLabelPool = nullptr;
	}

	if (m_pPools->pTextDrawPool) {
		delete m_pPools->pTextDrawPool;
		m_pPools->pTextDrawPool = nullptr;
	}

	if (m_pPools->pActorPool) {
		delete m_pPools->pActorPool;
		m_pPools->pActorPool = nullptr;
	}

	if (m_pPools->pMenuPool) {
		delete m_pPools->pMenuPool;
		m_pPools->pMenuPool = nullptr;
	}

	if (m_pPools->pPlayerBubblePool)
	{
		delete m_pPools->pPlayerBubblePool;
		m_pPools->pPlayerBubblePool = nullptr;
	}

	if (m_pPools) {
		delete m_pPools;
		m_pPools = nullptr;
	}

	if (m_pNetSet) {
		delete m_pNetSet;
		m_pNetSet = nullptr;
	}
}

void CNetGame::Process()
{
	// Enquanto a tela de "CONEXÃO PERDIDA" estiver aberta, congelamos a rede
	// por completo: nada de receber pacotes, nada de pools, nada de reconexão
	// automática. O servidor que reiniciou sozinho não consegue "reativar" o
	// jogo por trás do painel — só o clique do jogador decide.
	if (pUI && pUI->IsConnectionLost())
	{
		return;
	}

	static uint32_t time = GetTickCount();
	bool bProcess = false;
	if (GetTickCount() - time >= 1000 / 30)
	{
		UpdateNetwork();
		time = GetTickCount();
		bProcess = true;
	}
	if (m_pNetSet->byteHoldTime) {
		pGame->SetWorldTime(m_pNetSet->byteWorldTime_Hour, m_pNetSet->byteWorldTime_Minute);
	}

	//pGame->PreloadObjectsAnims();

	if (GetGameState() == GAMESTATE_CONNECTED) {
		ProcessPools();
	}
	else {
		ProcessLoadingScreen();
	}

	if (GetGameState() == GAMESTATE_WAIT_CONNECT && !(pUI && pUI->IsConnectionLost())) {
		ProcessConnecting();
	}
}

void CNetGame::UpdateNetwork()
{
	bool breakStatus = false;
	Packet *pkt = nullptr;
	unsigned char packetIdentifier;

	while (pkt = m_pRakClient->Receive())
	{
		packetIdentifier = GetPacketID(pkt);

		switch (packetIdentifier) {
            case ID_AUTH_KEY:
                Packet_AuthKey(pkt);
                break;

            case ID_CONNECTION_ATTEMPT_FAILED:
                Packet_ConnectAttemptFailed(pkt);
                break;

            case ID_NO_FREE_INCOMING_CONNECTIONS:
                Packet_NoFreeIncomingConnections(pkt);
                break;

            case ID_DISCONNECTION_NOTIFICATION:
                Packet_DisconnectionNotification(pkt);
                //SetGameState(GAMESTATE_WAIT_CONNECT);
                break;

            case ID_CONNECTION_LOST:
                Packet_ConnectionLost(pkt);
                break;

            case ID_CONNECTION_REQUEST_ACCEPTED:
                Packet_ConnectionSucceeded(pkt);
                break;

            case ID_FAILED_INITIALIZE_ENCRIPTION:
                Packet_FailedInitializeEncription(pkt);
                break;

            case ID_CONNECTION_BANNED:
                Packet_ConnectionBanned(pkt);
                SetGameState(GAMESTATE_WAIT_CONNECT);
                break;

            case ID_INVALID_PASSWORD:
                Packet_InvalidPassword(pkt);
                break;

            case ID_VEHICLE_SYNC:
                Packet_VehicleSync(pkt);
                break;

            case ID_AIM_SYNC:
                Packet_AimSync(pkt);
                break;

            case ID_BULLET_SYNC:
                Packet_BulletSync(pkt);
                break;

            case ID_PLAYER_SYNC:
                Packet_PlayerSync(pkt);
                break;

            case ID_MARKERS_SYNC:
                Packet_MarkerSync(pkt);
                break;

            case ID_UNOCCUPIED_SYNC:
                Packet_UnoccupiedSync(pkt);
                break;

            case ID_TRAILER_SYNC:
                Packet_TrailerSync(pkt);
                break;

            case ID_PASSENGER_SYNC:
                Packet_PassengerSync(pkt);
                break;

                        case Network::kRaknetPacketId: {
                Network::OnRaknetReceive(pkt);
                break;
            }
            case 251:
                Packet_CustomRPC(pkt);
                break;
            case 252:
                Packet_Phone(pkt);
                break;
            case 253:
                Packet_Character(pkt);
                break;
            case 254:
                Packet_Auth(pkt);
                break;

        }

		// Se um pacote acima abriu a tela de "CONEXÃO PERDIDA", paramos de
		// processar a fila imediatamente: pacotes restantes (ex.: aceite de
		// conexão ou RPCs de InitGame de um servidor que voltou) não podem
		// "reativar" o jogo por trás do painel.
		if (pUI && pUI->IsConnectionLost())
		{
			m_pRakClient->DeallocatePacket(pkt);
			break;
		}

		// voice
		/*if (!Network::OnRaknetReceive(*pkt)) {
			return;
		}*/


		m_pRakClient->DeallocatePacket(pkt);
	}
}

static void WriteAuthString(RakNet::BitStream& bs, const char* value)
{
    const char* text = value ? value : "";
    const uint8_t len = static_cast<uint8_t>(std::min<size_t>(255, strlen(text)));
    bs.Write(len);
    if (len) bs.Write(text, len);
}

void CNetGame::Packet_Auth(Packet *p)
{
    if (!p || p->length < 7 || !pJavaWrapper) return;
    RakNet::BitStream bs((unsigned char*)p->data, p->length, false);
    bs.IgnoreBits(8);
    uint32_t rpcID = 0; bs.Read(rpcID);
    if (rpcID == 3) { pJavaWrapper->HideLogin(); return; }
    uint16_t len = 0; bs.Read(len);
    if (len >= 2048) return;
    static char json[2048]; memset(json, 0, sizeof(json));
    if (len) bs.Read(json, len);
    if (rpcID == 1) pJavaWrapper->ShowLogin(json);
    else if (rpcID == 2) pJavaWrapper->AuthResult(json);
}

void CNetGame::SendAuthLogin(const char* email, const char* password)
{
    RakNet::BitStream bs; bs.Write((uint8_t)254); bs.Write((uint32_t)1);
    WriteAuthString(bs, email); WriteAuthString(bs, password);
    m_pRakClient->Send(&bs, SYSTEM_PRIORITY, RELIABLE, 0);
}

void CNetGame::SendAuthRegister(const char* fullname, const char* email, const char* password)
{
    RakNet::BitStream bs; bs.Write((uint8_t)254); bs.Write((uint32_t)2);
    WriteAuthString(bs, fullname); WriteAuthString(bs, email); WriteAuthString(bs, password);
    m_pRakClient->Send(&bs, SYSTEM_PRIORITY, RELIABLE, 0);
}

void CNetGame::Packet_Phone(Packet *p)
{
    if (!p || p->length < 2 || !pJavaWrapper) return;

    const uint8_t command = p->data[1];
    if (command == 1)
    {
        pJavaWrapper->ShowPhone();
    }
    else if (command == 2)
    {
        pJavaWrapper->HidePhone();
    }
}

// ============================================================
// CRIAÇÃO DE PERSONAGEM — servidor -> cliente (packet 253)
// Formato: [uint8 253][uint32 rpcID][uint16 len][json...]
//   rpcID 1: abrir tela de seleção (JSON com hasChar + visual)
//   rpcID 2: abrir tela de criação
//   rpcID 3: aplicar visual salvo e fechar janelas
//   rpcID 4: fechar janelas
// ============================================================
void CNetGame::Packet_Character(Packet *p)
{
    if (!p || p->length < 7) return;

    RakNet::BitStream bs((unsigned char *) p->data, p->length, false);
    bs.IgnoreBits(8); // skip packet id

    uint32_t rpcID;
    bs.Read(rpcID);

    uint16_t wJsonLen = 0;
    static char szJson[2048];

    switch (rpcID)
    {
        case 1: // OPEN_SELECT
        case 3: // APPLY_LOOK
        {
            bs.Read(wJsonLen);
            if (wJsonLen >= sizeof(szJson)) return;

            memset(szJson, 0, sizeof(szJson));
            if (wJsonLen > 0) {
                bs.Read(szJson, wJsonLen);
            }
            szJson[wJsonLen] = 0;

            FLog("[CHAR] RPC %u recebido: %s", rpcID, szJson);

            if (rpcID == 1) {
                CharacterCreator::OpenSelect(szJson);
            } else {
                CharacterCreator::ApplyLook(szJson);
            }
            break;
        }

        case 2: // OPEN_CREATE
            FLog("[CHAR] RPC 2 recebido (abrir criação)");
            CharacterCreator::OpenCreate();
            break;

        case 4: // CLOSE
            FLog("[CHAR] RPC 4 recebido (fechar)");
            CharacterCreator::CloseAll();
            break;
    }
}

void CNetGame::Packet_CustomRPC(Packet *p) {

    RakNet::BitStream bs((unsigned char *) p->data, p->length, false);
    bs.IgnoreBits(8); // skip packet id

    uint32_t rpcID;
    bs.Read(rpcID);

    switch (rpcID) {
        // ====================================================
        // WebUI BANCO — servidor -> cliente
        // rpcID 100: abrir interface | 103: atualizar dados
        // Payload: [uint8 len][len bytes de JSON]
        // ====================================================
        case 100:
        case 103: {
            extern CJavaWrapper* pJavaWrapper;

            uint16_t wJsonLen;
            bs.Read(wJsonLen);

            static char szJson[2048];
            if (wJsonLen >= sizeof(szJson)) return;

            memset(szJson, 0, sizeof(szJson));
            if (wJsonLen > 0) {
                bs.Read(szJson, wJsonLen);
            }
            szJson[wJsonLen] = 0;

            FLog("[BANK] RPC %u recebido: %s", rpcID, szJson);

            if (!pJavaWrapper) return;
            if (rpcID == 100) {
                pJavaWrapper->ShowBank(szJson);
            } else {
                pJavaWrapper->UpdateBank(szJson);
            }
            break;
        }

        // ====================================================
        // Botão flutuante de interação (proximidade do banco)
        // rpcID 105: mostrar (com alpha) | 106: esconder
        // Payload: [uint8 alpha] (apenas no 105)
        // ====================================================
        case 105: {
            extern CJavaWrapper* pJavaWrapper;
            uint8_t bAlpha = 255;
            bs.Read(bAlpha);
            FLog("[INTERACT] RPC 105 recebido, alpha=%d, pJavaWrapper=%p", bAlpha, (void*)pJavaWrapper);
            if (pJavaWrapper) {
                pJavaWrapper->ShowBankInteract((int)bAlpha);
            }
            break;
        }
        case 106: {
            extern CJavaWrapper* pJavaWrapper;
            FLog("[INTERACT] RPC 106 recebido (hide)");
            if (pJavaWrapper) {
                pJavaWrapper->HideBankInteract();
            }
            break;
        }

        case 99: {
            FLog("RPC_CHECK_CASH");
            uint8_t bLen, bLen1;
            uint16_t bVersion;
            char szText[30];
            char szText1[30];

            memset(szText, 0, 30);
            memset(szText1, 0, 30);

            bs.Read(bLen);
            if (bLen >= sizeof(szText) - 1)
                return;

            bs.Read(&szText[0], bLen);

            bs.Read(bLen1);
            if (bLen1 >= sizeof(szText1) - 1)
                return;

            bs.Read(&szText1[0], bLen1);

            bs.Read(bVersion);

            RwTexture *pCashTexture = nullptr;
            pCashTexture = (RwTexture *) CUtil::LoadTextureFromDB(szText1, szText);

            int iVersion;
            if (pCashTexture) {
                iVersion = bVersion;
                RwTextureDestroy(pCashTexture);
            } else iVersion = 0;

            RakNet::BitStream bsParams;

            bsParams.Write((uint8_t) 251);
            bsParams.Write((uint32_t) 99);

            bsParams.Write(iVersion);

            m_pRakClient->Send(&bsParams, SYSTEM_PRIORITY, RELIABLE, 0);

//			bsParams.Write(iVersion);
//			m_pRakClient->RPC(&RPC_CustomHash, &bsParams, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, NULL);
            break;
        }
    }
}

// ============================================================
// WebUI BANCO — cliente -> servidor (packet 251)
// Formato: [uint8 251][uint32 rpcID][payload]
//   rpcID 101 (registrar PIN): [uint32 pin]
//   rpcID 102 (transação):     [uint8 tipo][uint32 valor][uint16 destino][uint32 pin]
// ============================================================
void CNetGame::SendBankRegisterPin(int pin)
{
	RakNet::BitStream bs;
	bs.Write((uint8_t)251);
	bs.Write((uint32_t)101);
	bs.Write((uint32_t)pin);
	m_pRakClient->Send(&bs, SYSTEM_PRIORITY, RELIABLE, 0);
}

void CNetGame::SendBankVerifyPin(int pin)
{
	RakNet::BitStream bs;
	bs.Write((uint8_t)251);
	bs.Write((uint32_t)104);
	bs.Write((uint32_t)pin);
	m_pRakClient->Send(&bs, SYSTEM_PRIORITY, RELIABLE, 0);
}

void CNetGame::SendBankTransaction(uint8_t byteType, int iAmount, uint16_t wTarget, int pin, uint8_t source)
{
	RakNet::BitStream bs;
	bs.Write((uint8_t)251);
	bs.Write((uint32_t)102);
	bs.Write(byteType);
	bs.Write((uint32_t)iAmount);
	bs.Write(wTarget);
	bs.Write((uint32_t)pin);
	bs.Write(source); // 0=caixa eletronico, 1=celular
	m_pRakClient->Send(&bs, SYSTEM_PRIORITY, RELIABLE, 0);
}

// Jogador clicou "Acessar" no botão flutuante → pede pro servidor abrir o banco
void CNetGame::SendBankOpenRequest()
{
	RakNet::BitStream bs;
	bs.Write((uint8_t)251);
	bs.Write((uint32_t)100);
	m_pRakClient->Send(&bs, SYSTEM_PRIORITY, RELIABLE, 0);
}

// ============================================================
// CRIAÇÃO DE PERSONAGEM — cliente -> servidor (packet 253)
// Formato: [uint8 253][uint32 cmd][payload]
//   cmd 1 (modal confirmado): [uint8 genero][uint16 len][nome][uint16 len][sobrenome][uint8 dia][uint8 mes][uint16 ano]
//   cmd 2 (criação confirmada): [todos os uint8 do ClothesLook na ordem]
//   cmd 3 (NASCER): sem payload
// ============================================================
void CNetGame::SendCharCreateRequest(uint8_t gender, const char* name, const char* lastName, uint8_t day, uint8_t month, uint16_t year)
{
	RakNet::BitStream bs;
	bs.Write((uint8_t)253);
	bs.Write((uint32_t)1);
	bs.Write(gender);
	const char* n = name ? name : "";
	const char* l = lastName ? lastName : "";
	const uint8_t nameLen = static_cast<uint8_t>(strlen(n));
	bs.Write(nameLen);
	if (nameLen > 0) bs.Write(n, nameLen);
	const uint8_t lastNameLen = static_cast<uint8_t>(strlen(l));
	bs.Write(lastNameLen);
	if (lastNameLen > 0) bs.Write(l, lastNameLen);
	bs.Write(day);
	bs.Write(month);
	bs.Write(year);
	m_pRakClient->Send(&bs, SYSTEM_PRIORITY, RELIABLE, 0);
}

void CNetGame::SendCharSave(const ClothesLook& look)
{
	RakNet::BitStream bs;
	bs.Write((uint8_t)253);
	bs.Write((uint32_t)2);
	bs.Write(look.skinTone);
	bs.Write(look.face);
	bs.Write(look.hairStyle);
	bs.Write(look.hairColor);
	bs.Write(look.eyebrowStyle);
	bs.Write(look.eyebrowColor);
	bs.Write(look.beardStyle);
	bs.Write(look.beardColor);
	bs.Write(look.makeupStyle);
	bs.Write(look.makeupColor);
	bs.Write(look.topStyle);
	bs.Write(look.topColor);
	bs.Write(look.bottomStyle);
	bs.Write(look.bottomColor);
	bs.Write(look.shoesStyle);
	bs.Write(look.shoesColor);
	bs.Write(look.tattooArmLeft);
	bs.Write(look.tattooArmRight);
	bs.Write(look.tattooLegLeft);
	bs.Write(look.tattooLegRight);
	bs.Write(look.tattooBack);
	bs.Write(look.tattooFront);
	bs.Write(look.policeBelt);
	bs.Write(look.policeVest);
	bs.Write(look.policeHat);
	m_pRakClient->Send(&bs, SYSTEM_PRIORITY, RELIABLE, 0);
}

void CNetGame::SendCharSpawnRequest()
{
	RakNet::BitStream bs;
	bs.Write((uint8_t)253);
	bs.Write((uint32_t)3);
	m_pRakClient->Send(&bs, SYSTEM_PRIORITY, RELIABLE, 0);
}

// 0.3.7
void CNetGame::ShutdownForGameModeRestart()
{
	// voice
	SpeakerList::Hide();
	MicroIcon::Hide();
	Network::OnRaknetDisconnect();

	for (PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++)
	{
		CRemotePlayer* pRemotePlayer = GetPlayerPool()->GetAt(playerId);
		if (pRemotePlayer) {
			pRemotePlayer->SetTeam(NO_TEAM);
			pRemotePlayer->ResetAllSyncAttributes();
		}
	}

	GetPlayerPool()->GetLocalPlayer()->ResetAllSyncAttributes();
	GetPlayerPool()->GetLocalPlayer()->ToggleSpectating(false);
	GameResetStats();

	if (pAudioStream) { //add new
		pAudioStream->Stop(true);
	}

	SetGameState(GAMESTATE_RESTARTING);
	GetPlayerPool()->DeactivateAll();
	GetPlayerPool()->Process();
	ResetVehiclePool();
	ResetActorPool();
	ResetTextDrawPool();
	ResetGangZonePool();
	Reset3DTextLabelPool();
	ResetPickupPool();
	ResetObjectPool();
	ResetMenuPool();

	m_pNetSet->bDisableInteriorEnterExits = false;
	m_pNetSet->fNameTagDrawDistance = 70.0f;
	m_pNetSet->byteWorldTime_Hour = 12;
	m_pNetSet->byteWorldTime_Minute = 0;
	m_pNetSet->byteWeather = 1;
	m_pNetSet->byteHoldTime = 1;
	m_pNetSet->bNameTagLOS = true;
	m_pNetSet->bUseCJWalk = false;
	pGame->ToggleCJWalk(false);
	m_pNetSet->fGravity = 0.008f;
	m_pNetSet->iDeathDropMoney = 0;

	pGame->m_bCheckpointsEnabled = false;
	pGame->m_bRaceCheckpointsEnabled = false;

	pGame->FindPlayerPed()->m_pPed->SetInterior(0, true);
	pGame->ResetLocalMoney();
	pGame->FindPlayerPed()->SetDead();
	pGame->FindPlayerPed()->SetArmour(0.0f);
	pGame->EnableZoneNames(false);
	m_pNetSet->bZoneNames = false;
	GameResetRadarColors();
	pGame->SetGravity(m_pNetSet->fGravity);
	pGame->SetWantedLevel(0);
	pGame->EnableClock(false);

	// voice
	//SpeakerList::Hide();
	//MicroIcon::Hide();
	//Network::OnRaknetDisconnect();

	if (pUI) pUI->chat()->addInfoMessage("O servidor está reiniciando...");
}

int iVehiclePoolProcessFlag = 0;
void CNetGame::ProcessPools()
{
	if (GetPlayerPool()) {
		GetPlayerPool()->Process();
	}

	if(GetVehiclePool()) {
		GetVehiclePool()->Process();
	}

	if (GetPickupPool()) {
		GetPickupPool()->Process();
	}
}	
// 0.3.7
void CNetGame::ProcessLoadingScreen()
{
	// Esta rotina é executada continuamente enquanto a tela de carregamento está ativa.
	// Não reposicione o ped aqui: SetPosn chama CPed::Teleport e pode limpar/clonar
	// tarefas faciais repetidamente durante a conexão, causando SIGSEGV.
	CCamera::SetPosition(1438.0, -178.0, 90.0, 0.0, 0.0, 0.0);

	CCamera::LookAtPoint(1538.0, -1358.0, 20.0, 2);
	pGame->SetWorldWeather(1);
	pGame->DisplayHUD(false);
}
// 0.3.7
void CNetGame::ProcessConnecting()
{
	if (GetTickCount() - m_dwLastConnectAttempt > 100/*3000*/)
	{
		//if (pUI) pUI->chat()->addDebugMessage("Connecting to %s:%d...", m_szHostOrIp, m_iPort);
		

		m_pRakClient->Connect(m_szHostOrIp, m_iPort, 0, 0, 2);
		
		// voice fix voice not connect when restart
		Network::OnRaknetConnect(m_szHostOrIp, m_iPort);

		m_dwLastConnectAttempt = GetTickCount();
		SetGameState(GAMESTATE_CONNECTING);
	}
}
// 0.3.7
void gen_auth_key(char buf[260], char* auth_in);
void CNetGame::Packet_AuthKey(Packet *pkt)
{
	uint8_t byteAuthLen;
	char szAuth[260], szAuthKey[269];

	RakNet::BitStream bsAuth((unsigned char*)pkt->data, pkt->length, false);

	if (GetGameState() < GAMESTATE_WAIT_CONNECT || GetGameState() > GAMESTATE_AWAIT_JOIN) return;

	bsAuth.IgnoreBits(8);
	bsAuth.Read(byteAuthLen);
	bsAuth.Read(szAuth, byteAuthLen);
	szAuth[byteAuthLen] = '\0';

	gen_auth_key(szAuthKey, szAuth);
	uint8_t byteAuthKeyLen = strlen(szAuthKey);

	RakNet::BitStream bsKey;
	bsKey.Write((uint8_t)ID_AUTH_KEY);
	bsKey.Write(byteAuthKeyLen);
	bsKey.Write(szAuthKey, byteAuthKeyLen);
	m_pRakClient->Send(&bsKey, SYSTEM_PRIORITY, RELIABLE, 0);
}
// 0.3.7
void CNetGame::Packet_ConnectAttemptFailed(Packet *pkt)
{
	if (pUI) pUI->chat()->addDebugMessage("O servidor não respondeu. Tentando novamente...");
	if (pAudioStream) { //add new
		pAudioStream->Stop(true);
	}
	SpeakerList::Hide(); //add new
	MicroIcon::Hide();
	Network::OnRaknetDisconnect();
	SetGameState(GAMESTATE_WAIT_CONNECT);

	//SpeakerList::Hide();
	//MicroIcon::Hide();
}
// 0.3.7
void CNetGame::Packet_NoFreeIncomingConnections(Packet *pkt)
{
	if(pUI) pUI->chat()->addDebugMessage("Servidor sobrecarregado. Tentando novamente...");
	SpeakerList::Hide(); //addnew
	MicroIcon::Hide();
	Network::OnRaknetDisconnect();
	SetGameState(GAMESTATE_WAIT_CONNECT);

	//SpeakerList::Hide();
	//MicroIcon::Hide();
}
// 0.3.7
void CNetGame::Packet_DisconnectionNotification(Packet *pkt)
{
	if (pUI) pUI->chat()->addDebugMessage("O servidor encerrou a conexão.");
	if (pAudioStream) {
		pAudioStream->Stop(true);
	}
	m_pRakClient->Disconnect(2000);

	SpeakerList::Hide();
	MicroIcon::Hide();

    // O servidor encerrou a conexão: mostra a tela de "CONEXÃO PERDIDA"
    // para o jogador decidir quando reconectar. Nada de reset do mundo aqui.
    SetGameState(GAMESTATE_WAIT_CONNECT);
    if (pUI) pUI->ShowConnectionLost();
}
// 0.3.7
void CNetGame::Packet_ConnectionSucceeded(Packet *pkt)
{
	RakNet::BitStream bsSuccAuth(pkt->data, pkt->length, true);
	PLAYERID MyPlayerID;
	unsigned int uiChallenge;
	int iVersion = 4057;
	uint8_t byteMod = 1;

	bsSuccAuth.IgnoreBits(8);	// packetId
	bsSuccAuth.IgnoreBits(32);	// binaryAddress
	bsSuccAuth.IgnoreBits(16);	// port

	bsSuccAuth.Read(MyPlayerID);
	bsSuccAuth.Read(uiChallenge);
	uiChallenge ^= iVersion;

	

	SetGameState(GAMESTATE_AWAIT_JOIN);

	uint8_t byteNameLen = strlen(GetPlayerPool()->GetLocalPlayerName());
	uint8_t byteAuthBSLen = strlen(AUTH_BS);
	uint8_t byteClientVerLen = strlen(SAMP_VERSION);
    if(pSettings)
        byteClientVerLen = strlen(pSettings->Get().szVersion);


	RakNet::BitStream bsSend;
	bsSend.Write(iVersion);
	bsSend.Write(byteMod);
	bsSend.Write(byteNameLen);
	bsSend.Write(GetPlayerPool()->GetLocalPlayerName(), byteNameLen);
	bsSend.Write(uiChallenge);
	bsSend.Write(byteAuthBSLen);
	bsSend.Write(AUTH_BS, byteAuthBSLen);
	bsSend.Write(byteClientVerLen);
	//bsSend.Write(SAMP_VERSION, byteClientVerLen);
    if(pSettings)
        bsSend.Write(pSettings->Get().szVersion, byteClientVerLen);
    else
        bsSend.Write(SAMP_VERSION, byteClientVerLen);

	Network::OnRaknetRpc(RPC_ClientJoin, bsSend);

	m_pRakClient->RPC(&RPC_ClientJoin, &bsSend, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, nullptr);

	// voice
	SpeakerList::Hide();
	MicroIcon::Hide();
}
// 0.3.7
void CNetGame::Packet_FailedInitializeEncription(Packet *pkt)
{
    if (pUI) pUI->chat()->addDebugMessage("Não foi possível inicializar a criptografia.");
}
// 0.3.7
void CNetGame::Packet_ConnectionBanned(Packet *pkt)
{
    if (pUI) pUI->chat()->addDebugMessage("Você foi banido deste servidor.");
}
// 0.3.7
void CNetGame::Packet_InvalidPassword(Packet *pkt)
{
    if (pUI) pUI->chat()->addDebugMessage("Senha incorreta para conectar ao servidor.");
    m_pRakClient->Disconnect(0);
}
// 0.3.7
void CNetGame::Packet_ConnectionLost(Packet *pkt)
{
    if (m_pRakClient) {
        m_pRakClient->Disconnect(0);
    }

    if (pUI) pUI->chat()->addDebugMessage("Conexão com o servidor perdida.");

	// NÃO chamamos ShutdownForGameModeRestart aqui: ele mata o ped e dispara o
	// fade de morte do GTA (a tela escurece lentamente e depois "volta"). Como o
	// botão da tela leva ao launcher, o jogo deve apenas congelar na última cena.
	SetGameState(GAMESTATE_WAIT_CONNECT);
	if (pUI) pUI->ShowConnectionLost();

	if (pAudioStream) {
		pAudioStream->Stop(true);
	}

	SpeakerList::Hide();
	MicroIcon::Hide();
}
// 0.3.7
void CNetGame::Packet_PlayerSync(Packet *pkt)
{
	RakNet::BitStream bsData(pkt->data, pkt->length, false);
	ONFOOT_SYNC_DATA ofSync;
	uint32_t dwTime = 0;
	uint8_t bytePacketId;
	PLAYERID playerId;

	bool bHasLR, bHasUD;

	if (GetGameState() != GAMESTATE_CONNECTED) return;

	memset(&ofSync, 0, sizeof(ONFOOT_SYNC_DATA));

	bsData.Read(bytePacketId);
	bsData.Read(playerId);

	bsData.Read(bHasLR);
	if (bHasLR) {
		bsData.Read(ofSync.lrAnalog);
	}

	bsData.Read(bHasUD);
	if (bHasUD) {
		bsData.Read(ofSync.udAnalog);
	}

	bsData.Read(ofSync.wKeys);
	bsData.Read((char*)&ofSync.vecPos, sizeof(CVector));
	float w, x, y, z;
	bsData.ReadNormQuat(w, x, y, z);
	ofSync.quat.Set(x, y, z, w);

	uint8_t byteHealthArmour;
	uint8_t byteArmTemp = 0, byteHlTemp = 0;
	
	bsData.Read(byteHealthArmour);
	byteArmTemp = (byteHealthArmour & 0x0F);
	byteHlTemp = (byteHealthArmour >> 4);

	if (byteArmTemp == 0xF) ofSync.byteArmour = 100;
	else if (byteArmTemp == 0) ofSync.byteArmour = 0;
	else ofSync.byteArmour = byteArmTemp * 7;

	if (byteHlTemp == 0xF) ofSync.byteHealth = 100;
	else if (byteHlTemp == 0) ofSync.byteHealth = 0;
	else ofSync.byteHealth = byteHlTemp * 7;

	uint8_t byteCurrentWeapon = 0;
	bsData.Read(byteCurrentWeapon);
	ofSync.byteCurrentWeapon ^= (byteCurrentWeapon ^ ofSync.byteCurrentWeapon) & 0x3F;

	bsData.Read(ofSync.byteSpecialAction);
	bsData.ReadVector(ofSync.vecMoveSpeed.x, ofSync.vecMoveSpeed.y, ofSync.vecMoveSpeed.z);

	bool bHasVehicleSurfingInfo;
	bsData.Read(bHasVehicleSurfingInfo);
	if (bHasVehicleSurfingInfo)
	{
		bsData.Read(ofSync.wSurfID);
		bsData.Read(ofSync.vecSurfOffsets.x);
		bsData.Read(ofSync.vecSurfOffsets.y);
		bsData.Read(ofSync.vecSurfOffsets.z);
	}
	else
		ofSync.wSurfID = INVALID_VEHICLE_ID;

	bool bHasAnimation;
	bsData.Read(bHasAnimation);
	if (bHasAnimation) {
		bsData.Read(ofSync.dwAnimation);
	}
	else {
		ofSync.dwAnimation = 0x80000000;
	}

	CRemotePlayer *pRemotePlayer = GetPlayerPool()->GetAt(playerId);
	if (pRemotePlayer) {
		pRemotePlayer->StoreOnFootFullSyncData(&ofSync, 0);
	}
}
// 0.3.7
void CNetGame::Packet_VehicleSync(Packet* pkt)
{
	RakNet::BitStream bsData(pkt->data, pkt->length, false);
	uint32_t dwTime = 0;
	uint8_t bytePacketId;
	INCAR_SYNC_DATA icSync;
	PLAYERID PlayerID;

	if (GetGameState() != GAMESTATE_CONNECTED) return;

	memset(&icSync, 0, sizeof(INCAR_SYNC_DATA));

	bsData.Read(bytePacketId);
	bsData.Read(PlayerID);
	bsData.Read(icSync.VehicleID);
	bsData.Read(icSync.lrAnalog);
	bsData.Read(icSync.udAnalog);
	bsData.Read(icSync.wKeys);

	float w, x, y, z;
	bsData.ReadNormQuat(w, x, y, z);
	icSync.quat.Set(x, y, z, w);
	
	bsData.Read((char*)& icSync.vecPos, sizeof(CVector));
	bsData.ReadVector(icSync.vecMoveSpeed.x, icSync.vecMoveSpeed.y, icSync.vecMoveSpeed.z);

	// car health
	uint16_t wTempVehicleHealth;
	bsData.Read(wTempVehicleHealth);
	icSync.fCarHealth = (float)wTempVehicleHealth;

	// health/armour
	uint8_t byteHealthArmour;
	uint8_t byteArmTemp = 0, byteHlTemp = 0;

	bsData.Read(byteHealthArmour);
	byteArmTemp = (byteHealthArmour & 0x0F);
	byteHlTemp = (byteHealthArmour >> 4);

	if (byteArmTemp == 0xF) icSync.bytePlayerArmour = 100;
	else if (byteArmTemp == 0) icSync.bytePlayerArmour = 0;
	else icSync.bytePlayerArmour = byteArmTemp * 7;

	if (byteHlTemp == 0xF) icSync.bytePlayerHealth = 100;
	else if (byteHlTemp == 0) icSync.bytePlayerHealth = 0;
	else icSync.bytePlayerHealth = byteHlTemp * 7;

	// current weapon
	uint8_t byteTempWeapon;
	bsData.Read(byteTempWeapon);
	icSync.byteCurrentWeapon ^= (byteTempWeapon ^ icSync.byteCurrentWeapon) & 0x3F;

	bool bCheck;

	// siren
	bsData.Read(bCheck);
	if (bCheck) icSync.byteSirenOn = 1;
	// landinggear
	bsData.Read(bCheck);
	if (bCheck) icSync.byteLandingGearState = 1;
	// train speed
	bsData.Read(bCheck);
	if (bCheck) bsData.Read(icSync.fTrainSpeed);
	// triler id
	bsData.Read(bCheck);
	if (bCheck) bsData.Read(icSync.TrailerID);

	CRemotePlayer* pRemotePlayer = GetPlayerPool()->GetAt(PlayerID);
	if (pRemotePlayer) {
		pRemotePlayer->StoreInCarFullSyncData(&icSync, 0);
	}
}
// 0.3.7
void CNetGame::Packet_AimSync(Packet* pkt)
{
	RakNet::BitStream bsData(pkt->data, pkt->length, false);
	
	uint8_t bytePacketId;
	PLAYERID PlayerId;
	AIM_SYNC_DATA aimSync;
	bsData.Read(bytePacketId);
	bsData.Read(PlayerId);
	bsData.Read((char*)& aimSync, sizeof(AIM_SYNC_DATA));

	CRemotePlayer* pPlayer = m_pPools->pPlayerPool->GetAt(PlayerId);
	if (pPlayer)
		pPlayer->StoreAimFullSyncData(&aimSync);
}
// 0.3.7
void CNetGame::Packet_BulletSync(Packet* pkt)
{
	RakNet::BitStream bsData(pkt->data, pkt->length, false);

	if (GetGameState() != GAMESTATE_CONNECTED) return;

	BULLET_SYNC_DATA btSync;
	uint8_t bytePacketId = 0;
	PLAYERID PlayerID = 0;

	bsData.Read(bytePacketId);
	bsData.Read(PlayerID);
	bsData.Read((char*)&btSync, sizeof(BULLET_SYNC_DATA));

	CPlayerPool* pPlayerPool = GetPlayerPool();
	CRemotePlayer* pRemotePlayer = pPlayerPool->GetAt(PlayerID);
	if (pRemotePlayer)
		pRemotePlayer->StoreBulletFullSyncData(&btSync);

}
// 0.3.7
void CNetGame::Packet_PassengerSync(Packet* pkt)
{
	RakNet::BitStream bsData(pkt->data, pkt->length, false);

	if (GetGameState() == GAMESTATE_CONNECTED)
	{
		uint8_t bytePacketId;
		PLAYERID PlayerID;
		PASSENGER_SYNC_DATA psSync;

		bsData.Read(bytePacketId);
		bsData.Read(PlayerID);
		bsData.Read((char*)& psSync, sizeof(PASSENGER_SYNC_DATA));

		CRemotePlayer* pRemotePlayer = GetPlayerPool()->GetAt(PlayerID);
		if (pRemotePlayer) {
			pRemotePlayer->StorePassengerFullSyncData(&psSync);
		}
	}
}

// 0.3.7
void CNetGame::Packet_MarkerSync(Packet *pkt)
{
	if(m_iGameState != GAMESTATE_CONNECTED) return;

	uint8_t bytePacketId;
	int	iNumberOfPlayers;
	PLAYERID playerId;
	bool bIsPlayerActive;
	short sPos[3];

	RakNet::BitStream bsMarkersSync((unsigned char *)pkt->data, pkt->length, false);
	bsMarkersSync.Read(bytePacketId);
	bsMarkersSync.Read(iNumberOfPlayers);
	if(iNumberOfPlayers)
	{
		for(int i = 0; i < iNumberOfPlayers; i++)
		{
			bsMarkersSync.Read(playerId);
			bsMarkersSync.ReadCompressed(bIsPlayerActive);

			if(bIsPlayerActive)
			{
				bsMarkersSync.Read(sPos[0]);
				bsMarkersSync.Read(sPos[1]);
				bsMarkersSync.Read(sPos[2]);
			}

			if(playerId >= 0 && playerId < MAX_PLAYERS && GetPlayerPool()->GetSlotState(playerId))
			{
				CRemotePlayer *pPlayer = GetPlayerPool()->GetAt(playerId);
				if(pPlayer)
				{
					if(bIsPlayerActive)
						pPlayer->ShowGlobalMarker(sPos[0], sPos[1], sPos[2]);
					else
						pPlayer->HideGlobalMarker();
				}
			}
		}
	}
}

/*
void CNetGame::Packet_VoiceChannelOpenReply(Packet* pkt)
{
	FLog("Packet_VoiceChannelOpenReply");

	if (!pVoice || !GetGameState() == GAMESTATE_CONNECTED) return;

	GetPlayerPool()->GetLocalPlayer()->VoiceChannelAccept();
}*/

/*
void CNetGame::Packet_VoiceData(Packet* pkt)
{
	if (!pVoice || !GetGameState() == GAMESTATE_CONNECTED) return;

	PLAYERID playerId;
	int size;
	unsigned char data[MAX_VOICE_PACKET_SIZE];
	RakNet::BitStream bsData(pkt->data, pkt->length, false);
	bsData.IgnoreBits(8);
	bsData.Read(playerId);
	bsData.Read(size);
	bsData.Read((char*)data, size);

	if (GetPlayerPool()->GetSlotState(playerId)) {
		pVoice->Push(playerId, data, size);
	}
}*/

// 0.3.7
void CNetGame::UpdatePlayerScoresAndPings()
{
	static uint32_t dwLastUpdateTick = 0;

	if (GetTickCount() - dwLastUpdateTick > 3000) {
		dwLastUpdateTick = GetTickCount();

		RakNet::BitStream bsSend;
		m_pRakClient->RPC(&RPC_UpdateScoresPingsIPs, &bsSend, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
	}
}

void CNetGame::SendDialogResponse(uint16_t wDialogID, uint8_t byteButtonID, uint16_t wListBoxItem, const char* szInput)
{
	if (GetGameState() != GAMESTATE_CONNECTED) return;

	uint8_t length = strlen(szInput);

	RakNet::BitStream bsSend;
	bsSend.Write(wDialogID);
	bsSend.Write(byteButtonID);
	bsSend.Write(wListBoxItem);
	bsSend.Write(length);
	bsSend.Write(szInput, length);
	m_pRakClient->RPC(&RPC_DialogResponse, &bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
}

void CNetGame::SendChatMessage(const char* szMsg)
{
	if (GetGameState() != GAMESTATE_CONNECTED) return;

	RakNet::BitStream bsSend;
	uint8_t byteTextLen = strlen(szMsg);

	bsSend.Write(byteTextLen);
	bsSend.Write(szMsg, byteTextLen);

	m_pRakClient->RPC(&RPC_Chat, &bsSend, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, NULL);
}

void CNetGame::SendChatCommand(const char* szCommand)
{
	if (GetGameState() != GAMESTATE_CONNECTED) return;

	RakNet::BitStream bsParams;
	int iStrlen = strlen(szCommand);

	bsParams.Write(iStrlen);
	bsParams.Write(szCommand, iStrlen);
	m_pRakClient->RPC(&RPC_ServerCommand, &bsParams, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, NULL);
}
// 0.3.7
void CNetGame::SetMapIcon(uint8_t byteIconID, float fPosX, float fPosY, float fPosZ, uint8_t byteType, uint32_t dwColor, uint8_t byteStyle)
{
	if (m_dwMapIcon[byteIconID] != 0) {
		DisableMapIcon(byteIconID);
	}

		m_dwMapIcon[byteIconID] = pGame->CreateRadarMarkerIcon(byteType, fPosX, fPosY, fPosZ, dwColor, byteStyle);
	NexionMiniMap::SetServerBlip(byteIconID, fPosX, fPosY, fPosZ, byteType, dwColor, byteStyle);

}
// 0.3.7
void CNetGame::DisableMapIcon(uint8_t byteIconID)
{
	ScriptCommand(&disable_marker, m_dwMapIcon[byteIconID]);
	m_dwMapIcon[byteIconID] = 0;
	NexionMiniMap::RemoveServerBlip(byteIconID);
}
// 0.3.7
void CNetGame::ResetVehiclePool()
{
	if (m_pPools->pVehiclePool) {
		delete m_pPools->pVehiclePool;
	}

	m_pPools->pVehiclePool = new CVehiclePool();
}
// 0.3.7
void CNetGame::ResetActorPool()
{
	if (m_pPools->pActorPool) {
		delete m_pPools->pActorPool;
	}

	m_pPools->pActorPool = new CActorPool();
}
// 0.3.7
void CNetGame::ResetTextDrawPool()
{
	if (m_pPools->pTextDrawPool) {
		delete m_pPools->pTextDrawPool;
	}

	m_pPools->pTextDrawPool = new CTextDrawPool();
}
// 0.3.7
void CNetGame::ResetGangZonePool()
{
	if (m_pPools->pGangZonePool) {
		delete m_pPools->pGangZonePool;
	}

	m_pPools->pGangZonePool = new CGangZonePool();
}
// 0.3.7
void CNetGame::Reset3DTextLabelPool()
{
	if (m_pPools->pTextLabelPool) {
		delete m_pPools->pTextLabelPool;
	}

	m_pPools->pTextLabelPool = new C3DTextLabelPool();
}
// 0.3.7
void CNetGame::ResetMapIcons()
{
		for (int i = 0; i < MAX_MAP_ICONS; i++)
	{
		if (m_dwMapIcon[i]) {
			ScriptCommand(&disable_marker, m_dwMapIcon[i]);
			m_dwMapIcon[i] = 0;
		}
	}
	NexionMiniMap::ClearServerBlips();
}

// 0.3.7
void CNetGame::ResetPickupPool()
{
	if (m_pPools->pPickupPool) {
		delete m_pPools->pPickupPool;
	}

	m_pPools->pPickupPool = new CPickupPool();
}
// 0.3.7
void CNetGame::ResetObjectPool()
{
	if (m_pPools->pObjectPool) {
		delete m_pPools->pObjectPool;
	}

	m_pPools->pObjectPool = new CObjectPool();
}
// 0.3.7
void CNetGame::ResetMenuPool()
{
	if (m_pPools->pMenuPool) {
		delete m_pPools->pMenuPool;
	}

	m_pPools->pMenuPool = new CMenuPool();
}
// 0.3.7
void CNetGame::InitGameLogic()
{
	if (m_pNetSet->bManualVehicleEngineAndLight) {
		//InstallVehicleEngineLightPatches();
	}

	m_pNetSet->fWorldBounds[0] = 20000.0f;
	m_pNetSet->fWorldBounds[1] = -20000.0f;
	m_pNetSet->fWorldBounds[2] = 20000.0f;
	m_pNetSet->fWorldBounds[3] = -20000.0f;
}

void CNetGame::SendPrevClass()
{
	CPlayerPool* pPlayerPool = GetPlayerPool();
	if (pPlayerPool)
	{
		pPlayerPool->GetLocalPlayer()->SendPrevClass();
	}
}

void CNetGame::SendSpawn()
{
	CPlayerPool* pPlayerPool = GetPlayerPool();
	if (pPlayerPool)
	{
		pPlayerPool->GetLocalPlayer()->SendSpawn();
	}
}

void CNetGame::SendNextClass()
{
	CPlayerPool* pPlayerPool = GetPlayerPool();
	if (pPlayerPool)
	{
		pPlayerPool->GetLocalPlayer()->SendNextClass();
	}
}

void CNetGame::Packet_TrailerSync(Packet *pkt)
{
	if(GetGameState() != GAMESTATE_CONNECTED) return;
	
	uint8_t bytePacketId;
	PLAYERID playerId;

	TRAILER_SYNC_DATA trSync;
	memset(&trSync, 0, sizeof(TRAILER_SYNC_DATA));

	RakNet::BitStream bsTrailerSync((unsigned char *)pkt->data, pkt->length, false);
	bsTrailerSync.Read(bytePacketId);
	bsTrailerSync.Read(playerId);
	bsTrailerSync.Read((char*)&trSync, sizeof(TRAILER_SYNC_DATA));

	if(GetPlayerPool()) 
	{
		CRemotePlayer *pPlayer = GetPlayerPool()->GetAt(playerId);
		if(pPlayer)
			pPlayer->StoreTrailerFullSyncData(&trSync);
	}
}

void CNetGame::Packet_UnoccupiedSync(Packet *pkt)
{
	if(m_iGameState != GAMESTATE_CONNECTED) return;

	uint8_t bytePacketId;
	PLAYERID playerId;

	UNOCCUPIED_SYNC_DATA unocSync;
	memset(&unocSync, 0, sizeof(UNOCCUPIED_SYNC_DATA));

	RakNet::BitStream bsUnocSync((unsigned char *)pkt->data, pkt->length, false);
	bsUnocSync.Read(bytePacketId);
	bsUnocSync.Read(playerId);
	bsUnocSync.Read((char*)&unocSync,sizeof(UNOCCUPIED_SYNC_DATA));

	if(GetPlayerPool())
	{
		CRemotePlayer *pPlayer = GetPlayerPool()->GetAt(playerId);
		if(pPlayer)
			pPlayer->StoreUnoccupiedSyncData(&unocSync);
	}
}