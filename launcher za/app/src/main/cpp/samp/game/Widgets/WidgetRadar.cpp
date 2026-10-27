#include "WidgetRadar.h"
#include "vendor/armhook/patch.h"

// O mapa original do GTA abre quando o radar nativo fica "segurado" por alguns
// instantes (CWidgetRadar::IsHeldDown compara o tempo de toque com o limite de
// abertura). Como a interface Nexion substitui o radar por completo, o radar
// nativo nunca pode ser segurado: retornar false aqui impede que o mapa original
// abra por toque prolongado, inclusive com toques que sobrevivem à pausa do app
// (minimizar/voltar do segundo plano).
bool (*CWidgetRadar__IsHeldDown)(CWidgetRadar* thiz);
bool CWidgetRadar__IsHeldDown_hook(CWidgetRadar* thiz)
{
    (void)thiz;
    return false;
}

void CWidgetRadar::InjectHooks() {
    CHook::InlineHook("_ZN12CWidgetRadar10IsHeldDownEv", &CWidgetRadar__IsHeldDown_hook, &CWidgetRadar__IsHeldDown);
}
