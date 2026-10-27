# Integração do celular Flutter na sourcenova

Esta alteração adiciona o shell visual baseado no `CELLEMFLUTTER.zip`, usando os assets do `phone.zip`, dentro da Activity principal do launcher SA-MP. O telefone não é um aplicativo Android separado: ele é renderizado como uma interface Flutter sobre o jogo quando o cliente recebe um comando do servidor.

## Por que o telefone é entregue como AAR

O host original usa Android Gradle Plugin 8.2.1 e Gradle 8.9. O Flutter 3.47.1 instalado para criar o módulo utiliza AGP 9.1.0 e Gradle 9.3.1 no projeto gerado. Para não migrar toda a sourcenova e correr risco de quebrar dependências antigas, o host consome o telefone por AAR.

O código Flutter continua em `flutter_phone/`. A constraint do módulo foi ajustada para `>=3.12.0 <4.0.0`, portanto o Dart 3.12.2 apresentado no terminal do usuário é aceito. Não é necessário executar `flutter upgrade` para resolver este erro. Sempre que o Dart ou algum asset mudar, gere novamente os AARs antes de compilar o host.

## Build do módulo Flutter

No diretório `flutter_phone`, com Flutter instalado e Android SDK configurado:

```bash
flutter pub get
flutter build aar
```

O comando gera o repositório local em:

```text
flutter_phone/build/host/outputs/repo
```

No Windows, execute os mesmos comandos no PowerShell ou no terminal do Android Studio. A variável `ANDROID_HOME` ou a configuração equivalente do Android Studio precisa apontar para um SDK válido.

## Build do launcher

O `app/build.gradle` usa `compileSdk 36` porque o AAR Flutter 3.47 declara essa exigência. O `targetSdk 34` e o `minSdk 26` continuam iguais; alterar `compileSdk` não muda sozinho o comportamento de runtime nem a versão mínima dos aparelhos.

Depois da geração do AAR, abra a raiz da sourcenova no Android Studio ou execute:

```bash
gradlew.bat :app:assembleDebug
```

A configuração da raiz já aponta para o repositório local do Flutter e para `https://storage.googleapis.com/download.flutter.io`. O host usa `flutter_debug` no build debug e `flutter_release` no build release.

O sandbox desta preparação não possui Android SDK, portanto a compilação Android final não pôde ser executada aqui. O Flutter isolado foi validado com `flutter test` e `flutter analyze`; a tentativa de Gradle foi interrompida somente pela ausência do SDK Android.

## Protocolo SA-MP

O pacote do telefone usa o ID livre `252` (`0xFC`) e não reutiliza o pacote `251` do banco. O formato é exatamente:

```text
[uint8 packetId = 252][uint8 command]
```

Os comandos definidos são:

| Comando | Valor | Ação |
|---|---:|---|
| `PHONE_CMD_SHOW` | `1` | Exibe o telefone e bloqueia o controle do ped. |
| `PHONE_CMD_HIDE` | `2` | Solicita o fechamento e libera o controle do ped. |

O include PAWN está em `GmLurion/pawno/include/phone.inc` e também em `GmLurion/include/phone.inc`. O gamemode inclui `phone.inc` depois de `Pawn.RakNet`.

## Teste no servidor

Com o gamemode recompilado e o cliente atualizado:

```text
/phone
```

abre o telefone. O botão de fechar dentro da interface Flutter ou o botão voltar do Android fecha a interface. Para testar o sentido servidor → cliente:

```text
/phoneoff
```

O `phone.inc` usa `PR_SendPacket`, `BS_New`, `BS_WriteValue` e `BS_Delete`, seguindo a API atual do `Pawn.RakNet`.

## Controle do jogo

Quando o cliente recebe o comando `1`, o código nativo executa `TogglePlayerControllable(false)`. Quando a interface Flutter fecha, Java chama `nativeOnPhoneClosed`; esse callback apenas sinaliza a restauração. O loop nativo do jogo executa `TogglePlayerControllable(true)` na própria thread do jogo, evitando mexer no ped diretamente da thread de UI.

O botão voltar é tratado nesta ordem: primeiro fecha o Flutter se o telefone estiver visível; caso contrário, mantém o comportamento original do launcher. O callback também é chamado quando a Activity é destruída para evitar que o ped fique congelado.

## Arquivos principais

| Caminho | Função |
|---|---|
| `flutter_phone/lib/main.dart` | Base CELLEMFLUTTER portada: Home, gaveta de apps, telas internas, navegação, troca de wallpaper e ponte `MethodChannel`. |
| `flutter_phone/assets/` | Moldura, wallpapers, ícones e telas vindos de `phone.zip`. |
| `settings.gradle` | Mantém o host sem subprojeto Flutter moderno e usa o AAR local. |
| `build.gradle` | Adiciona os repositórios local do AAR e do engine Flutter. |
| `app/build.gradle` | Liga `flutter_debug` e `flutter_release` por variante. |
| `app/src/main/java/com/lurion/rp/SAMP.java` | Hospeda o `FlutterFragment` sobre o jogo. |
| `app/src/main/cpp/samp/net/netgame.cpp` | Recebe o pacote `252`. |
| `app/src/main/cpp/samp/main.cpp` | Callback JNI e restauração do controle do ped. |
| `GmLurion/pawno/include/phone.inc` | Funções PAWN `Phone_Show` e `Phone_Hide`. |
| `GmLurion/gamemodes/GM.pwn` | Comandos de teste `/phone` e `/phoneoff`. |

A base portada já organiza os apps em telas internas próprias: toque no ícone abre o app, a seta da barra superior retorna à Home, e o botão voltar do Android fecha primeiro a tela interna e só depois solicita o fechamento do telefone. A próxima etapa funcional é substituir os dados demonstrativos por protocolos do servidor, começando por mensagens/WhatsApp e depois conectando o banco existente ao aplicativo Flutter.

## Referências

[1] [Flutter — Integrate a Flutter module into your Android project](https://docs.flutter.dev/add-to-app/android/project-setup).

[2] [Flutter — What’s new in Flutter 3.47](https://flutter.dev/blog/whats-new-in-flutter-3-47).

[3] [Flutter — Deprecated imperative apply of Flutter’s Gradle plugins](https://docs.flutter.dev/release/breaking-changes/flutter-gradle-plugin-apply).

## Base CELLEMFLUTTER portada

O módulo integrado agora usa a arquitetura do CELLEMFLUTTER como referência principal. A `HomeScreen` apresenta wallpaper, relógio, clima, grade de apps e dock; o `AppWindow` seleciona uma tela própria para WhatsApp, Instagram, Banco, Uber, Câmera, Telefone, OLX, Ajustes, Mensagens, Galeria, Calculadora e Notas. A moldura é aplicada pelo `DeviceFrame`, que preserva a área interna do frame original e posiciona o aparelho à direita quando o host está em paisagem.

## Correções recentes de compatibilidade e visual

O host Java usa `io.flutter.embedding.android.RenderMode` diretamente. Em algumas versões do embedding, `RenderMode` não é uma classe interna de `FlutterFragment`; por isso a chamada correta é `.renderMode(RenderMode.texture)`.

No Flutter, o recorte da tela é calculado proporcionalmente ao tamanho renderizado do frame original de 814 × 1716 pixels. A área interna usa as margens do asset em escala, em vez de aplicar valores fixos em pixels lógicos. O wallpaper é renderizado dentro desse recorte e a moldura permanece como camada superior não interativa.

## Assets no host Android

Além dos assets declarados em `flutter_phone/pubspec.yaml`, esta entrega mantém uma cópia em `app/src/main/assets/flutter_assets/`. Essa cópia serve como fallback para o host legado: contém a moldura, os wallpapers, os ícones, as telas dos apps e as fontes usadas pela interface. Ela deve ser mantida quando o launcher for compilado pelo Gradle.

Se o AAR for regenerado corretamente, o Flutter também empacota os mesmos arquivos. A cópia no host evita que uma configuração antiga do repositório Maven local resulte em uma tela preta ou em ícones de fallback enquanto a integração ainda está sendo estabilizada.

## Correção estrutural do overlay

O telefone não deve ser anexado com `addContentView()` em uma camada paralela à árvore do jogo. A `SAMP` agora usa o `ui_layout` criado por `main_render_screen.xml`, que fica acima do `SurfaceView` do GTA. O `FlutterFragment` é inserido em um `FrameLayout` filho desse overlay, com `RenderMode.texture` e `TransparencyMode.transparent`. A classe base `NvEventQueueActivity` expõe esse container como `protected` para a Activity principal.

A entrega também inclui `app/src/main/assets/flutter_assets/` com o bundle necessário para o host: `AssetManifest.bin`, `AssetManifest.bin.json`, `FontManifest.json`, fontes, moldura, wallpapers, ícones e telas dos aplicativos. Essa cópia é intencional e não deve ser removida enquanto a integração AAR estiver sendo estabilizada.

A referência oficial confirma que um `FlutterFragment` deve ser adicionado a um `FrameLayout` existente, que um engine cacheado deve ser pré-aquecido antes de ser usado e que `RenderMode.texture` é apropriado quando a interface precisa ser posicionada na hierarquia de Views Android: https://docs.flutter.dev/add-to-app/android/add-flutter-fragment.
