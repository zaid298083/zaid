# Histórico completo do celular Flutter embutido no cliente Android SA-MP/GTA

**Projeto:** cliente Android Sourcenova/SA-MP com celular Flutter embutido  
**Pacote Android:** `com.nexion.online`  
**Data do relatório:** 27 de agosto de 2026  
**Status:** integração funcional em desenvolvimento; a validação definitiva depende de compilação e teste no APK em aparelho real.

## 1. Objetivo geral

O objetivo deste trabalho foi integrar um celular construído em Flutter dentro do cliente Android do GTA/SA-MP. O celular não é um aplicativo Android separado: ele é aberto dentro do jogo, permanece sobre a tela do GTA e é acionado pelo servidor por meio do comando `/phone` e da ponte nativa já existente.

A estrutura desejada foi baseada em um telefone visual semelhante ao material de referência fornecido: moldura/frame do aparelho, wallpaper, tela inicial, ícones de aplicativos, navegação entre páginas, animações de abertura e fechamento e aplicativos simulados com aparência própria.

O requisito principal de interação permaneceu ativo durante o desenvolvimento: o jogador deve conseguir movimentar o personagem enquanto o telefone está aberto. Por isso, não foi mantido nenhum bloqueio permanente de controle do jogador, nem foi reintroduzido `TogglePlayerControllable(false)`.

## 2. Estado inicial e mudança de direção

Antes da fase do celular, o projeto havia passado por uma investigação do sistema de customização de roupas/`ClothesSync`. Essa parte foi deixada de lado quando o objetivo mudou para o celular Flutter usando a source original da Sourcenova, sem a customização anterior.

A partir daí, o trabalho passou a utilizar a integração Flutter já existente no projeto do usuário como base visual e funcional, em vez de criar um aplicativo externo. O Flutter foi incorporado ao cliente Android por meio de um AAR e de um `FlutterFragment` hospedado pela Activity do GTA.

## 3. Integração Flutter dentro do Android

O host Android do celular está no arquivo:

```text
app/src/main/java/com/lurion/rp/SAMP.java
```

O pacote efetivo utilizado pelo projeto é `com.nexion.online`, embora o arquivo Java permaneça no caminho `com/lurion/rp/SAMP.java`.

A integração utiliza um engine Flutter em cache e um fragmento Flutter dentro de um container Android. O fluxo geral é:

1. O servidor ou o comando do jogo solicita a abertura do telefone.
2. A Activity cria ou exibe o container do telefone.
3. O `FlutterFragment` com engine em cache é colocado dentro do container.
4. O Flutter desenha a interface do telefone.
5. O Android mantém a ponte `MethodChannel` para comandos nativos, como abertura/fechamento, troca do modo de câmera e captura do frame GTA.

A configuração visual aprovada posicionou o celular no lado direito da tela, com pequeno afastamento das bordas superior e inferior e pequeno recuo para a esquerda. A geometria aprovada não deve ser alterada durante os próximos ajustes da câmera.

As medidas visuais consolidadas ficaram aproximadamente assim:

| Elemento | Estado aprovado |
|---|---|
| Posição | Canto direito da tela do GTA |
| Escala | Reduzida em relação à primeira versão |
| Margem lateral | Pequeno afastamento da borda direita |
| Área interna | Preenchida até as bordas do frame |
| Retângulo Flutter | `left=29`, `top=34`, `width=750`, `height=1604` |
| Raio visual | Aproximadamente `40` |
| Escala da imagem | `frameScale=0.96` |
| Recuo direito | `rightInset=12` |

## 4. Correções de compilação da integração Android/Flutter

Durante a integração apareceram alguns problemas de ambiente e compatibilidade.

### 4.1. Versão do Dart/Flutter

O projeto Flutter exigia uma versão do Dart superior à instalada no computador do usuário. O erro informava que o projeto exigia Dart `^3.13.1`, enquanto o ambiente tinha Dart `3.12.2`. A solução foi utilizar uma versão do Flutter compatível com a restrição do `pubspec` ou ajustar o ambiente Flutter antes de gerar o AAR.

### 4.2. Compile SDK do AAR

O AAR Flutter utilizado exigia compilação Android API 36 ou superior, enquanto o projeto estava em `compileSdk 34`. Esse problema foi identificado como incompatibilidade de metadados do AAR. A correção exigia alinhar a versão do Flutter/AAR e o `compileSdk` do aplicativo, evitando atualizar componentes de forma incompatível com a versão do Android Gradle Plugin usada pelo projeto.

### 4.3. RenderMode do FlutterFragment

Em uma versão da API Flutter utilizada pelo projeto, `FlutterFragment.RenderMode.texture` não existia com esse caminho. A referência correta passou a ser baseada no enum/API disponível na versão do Flutter embarcada, evitando o erro:

```text
cannot find symbol: variable RenderMode
```

### 4.4. Visibilidade do override Java

O método `onActivityResult` da classe `SAMP` estava declarado como `protected`, mas a classe base `GTASA` exigia uma implementação `public`. O erro foi resolvido igualando a visibilidade do método derivado à visibilidade da classe base.

### 4.5. JVM target Java/Kotlin

O Gradle acusou incompatibilidade entre Java 17 e Kotlin 21. O projeto passou a exigir que os targets Java/Kotlin fossem alinhados, evitando a falha de validação:

```text
Inconsistent JVM-target compatibility detected
```

## 5. Interface do celular

A interface foi evoluída para se aproximar de um telefone real, mantendo o celular dentro do jogo.

### 5.1. Tela inicial e frame

A tela inicial recebeu wallpaper, frame e bordas internas ajustadas. Foram corrigidos espaços brancos que apareciam em cima, embaixo e nas laterais quando o conteúdo Flutter não preenchia corretamente a área interna do frame.

A regra consolidada foi separar visualmente o frame externo da área de conteúdo interno, fazendo o conteúdo ocupar todo o retângulo interno sem ultrapassar as bordas arredondadas.

### 5.2. Barra superior e navegação

Foi ajustada a apresentação de horário, bateria e demais indicadores. A intenção foi que esses elementos permanecessem coerentes entre a tela inicial e os aplicativos, sem criar uma segunda barra incompatível ao abrir um app.

Também foram introduzidos três botões de navegação fixos do próprio telefone, em vez de adicionar setas e títulos específicos em cada aplicativo. A navegação lateral por páginas de aplicativos foi implementada para funcionar por arraste e não somente pelo clique nas bolinhas.

### 5.3. Animações

Os aplicativos passaram a usar uma animação inspirada em telefones reais: expansão/zoom do ícone para a tela do aplicativo, em vez de uma entrada agressiva sempre pelo lado esquerdo. A direção da animação deveria considerar a posição do ícone na tela inicial.

A animação foi tratada como uma transição de abertura/fechamento de aplicativo, com escala e deslocamento suaves. A implementação não deve bloquear o jogador nem interferir nos controles do GTA.

### 5.4. Aplicativos simulados

Foram trabalhadas telas simuladas de aplicativos, incluindo:

| Aplicativo/função | Estado trabalhado |
|---|---|
| WhatsApp | Interface visual baseada nas imagens de referência; contato de teste; fluxo de conversa e gravação de áudio em desenvolvimento |
| Instagram | Interface visual adaptada às referências fornecidas |
| Banco | Tela de aplicativo dentro do mesmo frame do telefone |
| Uber | Ícone e tela de aplicativo dentro da navegação do telefone |
| Telefone/discador | Teclado numérico posicionado na parte inferior |
| Galeria/câmera | Entrada para o sistema de câmera GTA integrado |

O usuário também pediu a remoção de referências visuais a `pablomodsx`/`pablo`, substituindo-as por `kalkcs`, e a redução do WhatsApp para apenas um contato de teste.

## 6. Fechamento do telefone

Foi definido que o botão central quadrado deve fechar o celular. O fechamento também deve chamar a limpeza nativa do modo de câmera, interromper a captura secundária e restaurar estados temporários da câmera principal quando aplicável.

A câmera frontal e a câmera traseira são modos do mesmo celular Flutter. Não existe um aplicativo Android independente para a câmera.

## 7. Primeira implementação da câmera GTA

A câmera do telefone não utiliza a câmera física do Android. Ela utiliza uma câmera RenderWare adicional dentro do GTA, chamada no código de `PhoneSecondaryCamera`.

Arquivos principais:

```text
app/src/main/cpp/samp/game/PhoneSecondaryCamera.h
app/src/main/cpp/samp/game/PhoneSecondaryCamera.cpp
app/src/main/cpp/samp/game/RW/RenderWare.h
app/src/main/cpp/samp/game/RW/RenderWare.cpp
app/src/main/java/com/lurion/rp/SAMP.java
```

A câmera secundária foi criada com:

| Recurso | Configuração |
|---|---|
| Raster | `256 x 432` |
| Formato | 32 bits, raster de textura de câmera |
| Z-buffer | Raster separado |
| Render target | `RwCamera` própria |
| Captura | RAW1 para arquivo temporário |
| Transporte | Ponte nativa para o Android |
| Conversão Android | RAW1 para `Bitmap ARGB_8888` |
| Cor | Conversão corrigida de RGBA para Bitmap |

## 8. Problema crítico do RenderQueue

A primeira tentativa de usar `glReadPixels` diretamente no `MainLoop` produzia FBO, viewport e leitura zerados. O resultado era uma imagem preta, mesmo quando a câmera RenderWare parecia corretamente criada.

A investigação mostrou que o GTA Mobile encaminha operações OpenGL por uma fila de renderização/`GraphicsThread`. A leitura direta não aguardava corretamente a execução do comando no contexto gráfico usado pelo jogo.

A solução foi resolver e chamar o símbolo interno:

```text
emu_glReadPixels
```

Essa ponte força a leitura sincronizada pelo caminho de renderização utilizado pelo GTA Mobile. Depois da mudança, os logs passaram a mostrar FBO/readback válido e o ped apareceu na câmera secundária.

## 9. Formato RAW1 e correção de cores

O nativo grava o frame com cabeçalho RAW1 contendo:

```text
magic
width
height
pixels RGBA
```

O Android lê o arquivo em `SAMP.java`, valida o cabeçalho e transforma cada quatro bytes em pixels `ARGB_8888`.

Um problema inicial trocava os canais de cor, fazendo objetos vermelhos aparecerem azuis e produzindo cores artificiais. A conversão foi corrigida para respeitar a ordem:

```text
R, G, B, A
```

A traseira passou então a apresentar cores naturais, com céu e objetos mais próximos da imagem do GTA.

## 10. Orientação da câmera RenderWare

A orientação da `RwCamera` foi corrigida para utilizar a convenção RenderWare:

```text
right = eixo lateral
up    = eixo vertical
at    = direção da câmera
```

A matriz passou a ser atualizada com `right`, `up`, `at` e posição coerentes. Uma tentativa anterior de trocar a ordem dos eixos fazia a imagem virar verticalmente ou ficar espelhada.

Para a traseira, foi mantido o espelhamento horizontal corrigido sem inverter o eixo vertical. A orientação de paisagem, árvores e objetos passou a ficar correta após esses ajustes.

## 11. Câmera traseira

O objetivo da câmera traseira é mostrar o mundo à frente do jogador, como uma câmera real apontada para a paisagem. Ela não deve mostrar o ped local no visor e, ao ser ativada, a imagem grande do GTA deveria entrar no modo primeira pessoa baseado no `build.zip`.

A câmera traseira passou a usar:

```text
FirstPersonOrigin(player, eyeFallback)
ActiveCameraForward(forward)
```

A origem tenta utilizar a matriz da cabeça, com validação de valores finitos e limite de distância. Quando a matriz óssea apresenta valores inválidos ou ainda não está pronta, é utilizado fallback para o pescoço ou para uma altura segura derivada da posição do jogador.

O mundo traseiro passou a ser coletado por:

```text
CRenderer::GetObjectsInFrustum
```

Offset arm64 utilizado na source:

```text
0x004F76AC
```

Esse caminho solucionou a ausência de árvores e objetos que não apareciam nas primeiras versões.

### 11.1. Problemas evitados na traseira

A chamada direta de `CRenderer::ScanWorld` causou crash no caminho:

```text
AddEntityToRenderList -> ScanSectorList -> CWorldScan::ScanWorld
```

Por isso, essa chamada não deve ser reintroduzida.

Também foi testado executar a câmera secundária dentro do hook `RenderEverythingBarRoads`. Isso provocou piscada da cidade porque o passe secundário interferia no estado global do RenderQueue/renderizador. A solução foi manter `gPhoneSecondaryCamera.Tick()` no `MainLoop`, fora desse hook gráfico.

## 12. Primeira pessoa da imagem grande do GTA

O `build.zip` fornecido pelo usuário contém `CFirstPersonCamera.cpp`. A análise mostrou que a primeira pessoa real não consiste apenas em esconder o ped no FBO. Ela altera a câmera principal do jogo, especialmente a origem da câmera pela matriz da cabeça e a direção armazenada em `CCam::Front`.

O hook de referência relevante é o pós-processamento de:

```text
CCam::Process
```

Símbolo utilizado na tentativa de portabilidade:

```text
_ZN4CCam7ProcessEv
```

O primeiro patch havia aplicado a alteração em `CCamera__Process`, mas o log mostrou que nenhum `main_fp` era emitido. Isso indicou que o ponto não era o objeto real que desenhava a imagem grande. Foi então preparado um hook separado para `CCam::Process`, chamando o processamento original e aplicando o ajuste depois.

O comportamento pretendido para a traseira é:

1. Processar a câmera normalmente.
2. Se o modo traseiro estiver ativo e o jogador estiver a pé, alterar `CCam::Source` para a origem de primeira pessoa.
3. Aplicar `MODE_1STPERSON`.
4. Usar near clip `0,2`, como na referência.
5. Restaurar o modo e o near clip ao trocar para frontal ou fechar o telefone.

No entanto, a validação definitiva dessa alteração depende de o usuário compilar e testar o APK correspondente. O log anterior não provou a troca da imagem grande; ele apenas provou a execução da câmera secundária.

## 13. Câmera frontal/selfie

A câmera frontal deveria mostrar o ped como uma selfie. Ela deve ser independente da alteração de primeira pessoa da imagem grande e não deve forçar a câmera principal do GTA.

Durante os testes, a frontal apresentou vários problemas:

| Sintoma | Tentativas realizadas |
|---|---|
| Ped fora do enquadramento | Ajustes de distância e campo de visão |
| Jogador aparecendo de costas | Troca do lado da lente em relação a `GetForward()` |
| Imagem de cabeça para baixo | Alterações na ordem vertical das linhas RAW1 |
| Ped deformado | Remoção do uso direto de uma matriz óssea instável no enquadramento frontal |
| Cidade/objetos sobre o ped | Renderização do mundo antes do ped e prioridade temporária do ped no passe frontal |
| Partes da cidade ausentes | Aumento da capacidade frontal da lista de entidades para até 8192 |
| Câmera de baixo para cima | Nivelamento entre altura da lente e altura do alvo |
| Jogador afastado | Redução da distância da lente para `1,35` e abertura da janela frontal |

A versão mais recente documentada utiliza, na frontal, uma posição derivada de uma altura fixa do jogador, sem depender do bone da cabeça durante o enquadramento. A lente fica no lado oposto ao vetor `GetForward()` observado na source, e o alvo fica na mesma altura para evitar a inclinação de baixo para cima.

Valores mais recentes utilizados na frontal:

```text
selfieCenter = playerPosition + (0, 0, 1.15)
distância = 1.35
viewWidth frontal = 0.52
alvo = selfieCenter
```

A coleta do mundo frontal foi parametrizada para usar capacidade de até 8192 entidades, enquanto a traseira continuou com a chamada padrão de 2048 entidades. O log frontal passou a informar:

```text
PHONE_CAM front_world drawn=... capacity=8192
```

Essa última configuração ainda precisa ser confirmada visualmente no APK. Não se deve considerar a frontal finalizada sem observar o visor no aparelho.

## 14. Principais offsets e símbolos utilizados

| Função/recurso | Offset ou símbolo | Finalidade |
|---|---:|---|
| `RpClumpRender` | `0x2BA6A4` | Renderização de clumps |
| `CRenderer::RenderOneNonRoad` | `0x4F56E0` | Renderização de entidades não-estrada |
| `CRenderer::GetObjectsInFrustum` | `0x4F76AC` | Coleta de entidades visíveis na frustum |
| `CVisibilityPlugins::SetRenderWareCamera` | `0x6FB5BC` | Seleção temporária da câmera RenderWare |
| `CPedGTA::GetBonePosition` | `0x59AEE4` | Obtenção da posição de bones |
| leitura sincronizada | `emu_glReadPixels` | Readback pelo RenderQueue/GraphicsThread |
| primeira pessoa | `_ZN4CCam7ProcessEv` | Hook real de processamento de `CCam` |

Os offsets acima são referentes ao alvo arm64 da versão da libGTASA estudada no projeto. Eles não devem ser aplicados automaticamente a outra versão do APK sem confirmação.

## 15. Arquivos principais envolvidos

### Flutter e Android

```text
app/src/main/java/com/lurion/rp/SAMP.java
```

Responsável pelo host Android do Flutter, `MethodChannel`, comandos de modo da câmera, fechamento do telefone e conversão do RAW1 em Bitmap.

### Câmera nativa

```text
app/src/main/cpp/samp/game/PhoneSecondaryCamera.h
app/src/main/cpp/samp/game/PhoneSecondaryCamera.cpp
```

Responsáveis pela câmera RenderWare secundária, criação de raster/z-buffer, transformações frontal/traseira, coleta do mundo, render do ped, readback e escrita do RAW1.

### Ponte RenderWare

```text
app/src/main/cpp/samp/game/RW/RenderWare.h
app/src/main/cpp/samp/game/RW/RenderWare.cpp
```

Declaram e resolvem funções RenderWare e a ponte `emu_glReadPixels`.

### Hook nativo

```text
app/src/main/cpp/samp/game/hooks.cpp
```

Contém hooks do cliente. Durante a investigação da primeira pessoa, foi testado o hook de `CCamera__Process` e depois identificado o caminho de `CCam::Process` baseado na referência do `build.zip`.

### Acesso à câmera

```text
app/src/main/cpp/samp/game/Camera.h
app/src/main/cpp/samp/game/Camera.cpp
app/src/main/cpp/samp/game/Cam.h
```

Contêm os layouts de `CCamera` e `CCam`, incluindo `m_nMode`, `Front`, `Source` e `Up`.

### Ped e bones

```text
app/src/main/cpp/samp/game/playerped.cpp
app/src/main/cpp/samp/game/Entity/CPedGTA.cpp
app/src/main/cpp/samp/game/Enums/ePedBones.h
```

Contêm atualização da matriz da cabeça, obtenção de posição de bones e enumeração de `BONE_NECK`, `BONE_HEAD` e demais ossos.

## 16. Regras que devem ser preservadas

As seguintes regras foram definidas para evitar regressões:

| Regra | Motivo |
|---|---|
| Não alterar a geometria/UI aprovada | O frame e o retângulo Flutter já foram ajustados visualmente |
| Não chamar `CRenderer::ScanWorld` diretamente | Caminho causou crash |
| Não executar a câmera secundária em `RenderEverythingBarRoads` | Caminho causou piscada da cidade |
| Manter `Tick()` no `MainLoop` | Foi o caminho estável para o RenderQueue |
| Não bloquear controles do jogador | O jogador deve continuar se movendo com o telefone aberto |
| Não tratar veículo nesta etapa | A primeira pessoa principal foi limitada ao jogador a pé |
| Não alterar a traseira durante ajustes frontais | A traseira chegou a um estado visual considerado quase perfeito |
| Entregar somente arquivos modificados | Evita enviar source completa, AAR, APK ou históricos desnecessários |

## 17. Procedimento de compilação usado pelo usuário

Para alterações somente em C++/Java, o procedimento definido foi:

```bat
gradlew.bat :app:assembleDebug
```

Não é necessário reconstruir o Flutter AAR quando `main.dart` não foi alterado. Quando houver mudança real no Flutter, o AAR precisa ser gerado novamente de acordo com a versão Flutter/Dart compatível do projeto.

## 18. Estado real no momento deste relatório

O celular Flutter está integrado ao cliente Android e possui frame, wallpaper, tela inicial, aplicativos, navegação, animações e ponte nativa de câmera em desenvolvimento.

A câmera secundária RenderWare já conseguiu produzir frames válidos, mostrar o ped, mostrar paisagem, árvores e objetos, corrigir cores e eliminar a piscada causada pelo passe gráfico no lugar errado.

A traseira possui o caminho de mundo considerado quase perfeito pelo usuário, mas a troca da imagem grande do GTA para primeira pessoa depende da confirmação do hook real de `CCam::Process` no APK que será testado. A frontal continua em ajuste fino: a posição, o lado da lente, o nivelamento e a cobertura do mundo foram modificados várias vezes, mas nenhuma dessas versões deve ser considerada definitivamente aprovada sem captura visual do aparelho.

Também houve uma inconsistência de assinatura durante a troca de patches. O header chegou a declarar:

```cpp
void ApplyMainFirstPerson(CCam* camera);
```

enquanto um `.cpp` antigo ainda continha:

```cpp
void CPhoneSecondaryCamera::ApplyMainFirstPerson();
```

Isso causou erro de compilação. A regra para evitar o problema é sempre substituir os arquivos do mesmo pacote juntos quando o patch alterar uma assinatura entre header, implementação e hook.

## 19. Conclusão

O projeto deixou de ser apenas uma interface Flutter visual e passou a ter uma integração nativa com o renderizador do GTA. A parte mais complexa foi a câmera, porque o jogo mobile usa RenderQueue/GraphicsThread e não permite tratar o frame como uma captura OpenGL comum do thread principal.

O caminho atual é modular: Flutter cuida do telefone e dos aplicativos; Java hospeda o Flutter e converte os frames; C++ cria e renderiza a câmera secundária; RenderWare fornece a câmera, o mundo e o ped; e os hooks são usados somente quando é necessário alterar o processamento interno do GTA.

O próximo procedimento seguro é testar cada modo separadamente no APK: primeiro frontal, verificando orientação, rosto, distância, chão e objetos; depois traseira, verificando paisagem e ausência do ped; por fim, a imagem grande do GTA, verificando se a primeira pessoa realmente é aplicada e restaurada ao fechar ou trocar de câmera.

## 20. Referências internas do projeto

Os principais materiais usados para reconstruir este histórico são os próprios arquivos do projeto e a referência entregue pelo usuário:

```text
/home/ubuntu/sourcenova_phone_work
/home/ubuntu/build_reference/app/src/main/cpp/multiplayer/game/CFirstPersonCamera.cpp
/home/ubuntu/build_reference/app/src/main/cpp/multiplayer/game/hooks.cpp
/home/ubuntu/upload/pasted_content_30.txt
/home/ubuntu/upload/pasted_content_31.txt
```

Este documento descreve o estado de desenvolvimento e não substitui a validação do APK no dispositivo Android real.
