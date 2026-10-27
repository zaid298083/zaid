import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';

import 'package:audioplayers/audioplayers.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:path_provider/path_provider.dart';
import 'package:record/record.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  PhoneBridge.initialize();
  runApp(const PhoneSimulator());
}

class PhoneBridge {
  static const channel = MethodChannel('com.nexion.online/phone');

  // Callbacks para dados vindos do nativo (Java -> Flutter)
  static void Function(String jsonData)? onBankData;
  static void Function(String jsonData)? onBankUpdate;
  static void Function(String location, String time)? onGameData;

  static void initialize() {
    channel.setMethodCallHandler((call) async {
      switch (call.method) {
        case 'onBankData':
          final json = call.arguments as String? ?? '{}';
          onBankData?.call(json);
          break;
        case 'onBankUpdate':
          final json = call.arguments as String? ?? '{}';
          onBankUpdate?.call(json);
          break;
        case 'onGameData':
          if (call.arguments is Map) {
            final map = Map<String, dynamic>.from(call.arguments as Map);
            final loc = map['location'] as String? ?? 'San Andreas';
            final time = map['time'] as String? ?? '00:00';
            onGameData?.call(loc, time);
          }
          break;
      }
    });
  }

  static Future<void> closePhone() async {
    try {
      await channel.invokeMethod<void>('closePhone');
    } on MissingPluginException {
      // Permite executar a base também fora do host Android.
    } on PlatformException {
      // O host pode estar sendo destruído durante o fechamento.
    }
  }

  static Future<void> setGtaCameraMode(int mode) async {
    try {
      await channel.invokeMethod<void>('setGtaCameraMode', mode);
    } on MissingPluginException {
      // A câmera nativa não existe no Web.
    } on PlatformException {
      // O host pode estar sendo destruído durante a troca de tela.
    }
  }

  static Future<Uint8List?> captureGtaCamera() async {
    try {
      final encoded = await channel.invokeMethod<String>('captureGtaCamera');
      if (encoded == null || encoded.isEmpty) return null;
      return base64Decode(encoded);
    } on MissingPluginException {
      return null;
    } on PlatformException {
      return null;
    }
  }

  // ==================== BANCO ====================
  /// Pede os dados do banco ao servidor (packet 251 rpcID 100)
  static Future<void> requestBankData() async {
    try {
      await channel.invokeMethod<void>('requestBankData');
    } on MissingPluginException {
      // Fora do host Android
    } on PlatformException {
      // Host pode estar sendo destruído
    }
  }

  /// Envia uma ação do banco ao servidor (saque, depósito, transferência, PIN)
  static Future<void> sendBankAction(String action, Map<String, dynamic> data) async {
    try {
      await channel.invokeMethod<void>('sendBankAction', {
        'action': action,
        'data': jsonEncode(data),
      });
    } on MissingPluginException {
      // Fora do host Android
    } on PlatformException {
      // Host pode estar sendo destruído
    }
  }
}

class PhoneSimulator extends StatefulWidget {
  const PhoneSimulator({super.key});

  @override
  State<PhoneSimulator> createState() => _PhoneSimulatorState();
}

class _PhoneSimulatorState extends State<PhoneSimulator> {
  String? _activeApp;
  Alignment _openAnchor = Alignment.centerRight;
  bool _showAppDrawer = false;
  int _homePage = 0;
  bool _darkMode = false;
  int _wallpaperIndex = 0;
  DateTime _now = DateTime.now();
  Timer? _clockTimer;
  final PageController _homePageController = PageController();

  // Dados do jogo (localização + hora do servidor)
  String _gameLocation = 'Los Santos';
  String _gameTime = '12:00';

  final List<String> _wallpapers = const [
    'assets/wallpapers/homeBg.png',
    'assets/wallpapers/homeBg2.png',
    'assets/wallpapers/homeBg3.png',
    'assets/wallpapers/homeBg4.png',
    'assets/wallpapers/homeBg5.png',
    'assets/wallpapers/homeBg6.png',
  ];

  final List<AppDefinition> _apps = const [
    AppDefinition(
      'whatsapp',
      'WhatsApp',
      'assets/apps/WhatsIcon.png',
      Color(0xFF25D366),
    ),
    AppDefinition(
      'instagram',
      'Instagram',
      'assets/apps/InstagramIcon.png',
      Color(0xFFE1306C),
    ),
    AppDefinition(
      'bank',
      'Banco',
      'assets/apps/BankIcon.png',
      Color(0xFF155EEF),
    ),
    AppDefinition(
      'uber',
      'Uber',
      'assets/apps/UberIcon.png',
      Color(0xFF111111),
    ),
    AppDefinition(
      'camera',
      'Câmera',
      'assets/apps/CameraIcon.png',
      Color(0xFF3F4653),
    ),
    AppDefinition(
      'phone',
      'Telefone',
      'assets/apps/PhoneIcon.png',
      Color(0xFF20B45B),
    ),
    AppDefinition('olx', 'OLX', 'assets/apps/OlxIcon.png', Color(0xFF6A35FF)),
    AppDefinition(
      'settings',
      'Ajustes',
      'assets/apps/SettingsIcon.png',
      Color(0xFF64748B),
    ),
    AppDefinition(
      'ifood',
      'iFood',
      'assets/apps/IfoodIcon.png',
      Color(0xFFEA1D2C),
    ),
    AppDefinition(
      'messages',
      'Mensagens',
      'assets/apps/MessagesIcon.png',
      Color(0xFF2397F3),
    ),
    AppDefinition(
      'gallery',
      'Galeria',
      'assets/apps/GaleryIcon.png',
      Color(0xFFFD8A24),
    ),
    AppDefinition(
      'calculator',
      'Calculadora',
      'assets/apps/CalculatorIcon.png',
      Color(0xFF263238),
    ),
    AppDefinition(
      'notes',
      'Notas',
      'assets/apps/NotesIcon.png',
      Color(0xFFFFC107),
    ),
    AppDefinition(
      'internet',
      'Internet',
      'assets/apps/InternetIcon.png',
      Color(0xFF1A73E8),
    ),
    AppDefinition(
      'playstore',
      'Play Store',
      'assets/apps/PlayStoreIcon.png',
      Color(0xFF1A73E8),
    ),
    AppDefinition(
      'racing',
      'Corrida',
      'assets/apps/RacingIcon.png',
      Color(0xFFEF4444),
    ),
    AppDefinition(
      'deepweb',
      'Deep Web',
      'assets/apps/DeepWebIcon.png',
      Color(0xFF171717),
    ),
  ];

  @override
  void initState() {
    super.initState();
    _clockTimer = Timer.periodic(const Duration(seconds: 30), (_) {
      if (mounted) setState(() => _now = DateTime.now());
    });
    PhoneBridge.onGameData = (location, time) {
      if (mounted) setState(() { _gameLocation = location; _gameTime = time; });
    };
  }

  @override
  void dispose() {
    _clockTimer?.cancel();
    PhoneBridge.onGameData = null;
    _homePageController.dispose();
    super.dispose();
  }

  void _openApp(String id, [Alignment anchor = Alignment.centerRight]) {
    setState(() {
      _activeApp = id;
      _openAnchor = anchor;
      _showAppDrawer = false;
    });
  }

  void _closeApp() => setState(() => _activeApp = null);

  Future<void> _closePhone() async {
    if (mounted) {
      setState(() {
        _activeApp = null;
        _showAppDrawer = false;
      });
    }
    await PhoneBridge.closePhone();
  }

  Future<bool> _handleBack() async {
    if (_showAppDrawer) {
      setState(() => _showAppDrawer = false);
      return false;
    }
    if (_activeApp != null) {
      _closeApp();
      return false;
    }
    // Na home screen, seta nao faz nada
    return false;
  }

  @override
  Widget build(BuildContext context) {
    final currentWallpaper = _wallpapers[_wallpaperIndex];
    final appById = {for (final app in _apps) app.id: app};
    final homePages = [
      [
        'uber',
        'ifood',
        'deepweb',
        'olx',
        'whatsapp',
        'notes',
        'calculator',
        'instagram',
      ],
      [
        'bank',
        'gallery',
        'internet',
        'playstore',
        'racing',
        'messages',
        'settings',
        'camera',
      ],
    ].map((page) => page.map((id) => appById[id]!).toList()).toList();
    final dockApps = [
      'settings',
      'messages',
      'phone',
      'camera',
    ].map((id) => appById[id]!).toList();

    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Lurion Phone',
      color: Colors.transparent,
      theme: ThemeData(
        useMaterial3: true,
        scaffoldBackgroundColor: Colors.transparent,
        brightness: _darkMode ? Brightness.dark : Brightness.light,
        colorScheme: ColorScheme.fromSeed(
          seedColor: const Color(0xFF5E5CE6),
          brightness: _darkMode ? Brightness.dark : Brightness.light,
        ),
      ),
      home: PopScope(
        canPop: false,
        onPopInvokedWithResult: (didPop, result) {
          if (!didPop) unawaited(_handleBack());
        },
        child: DeviceFrame(
          child: Column(
            children: [
              Expanded(
                child: AnimatedSwitcher(
                  duration: const Duration(milliseconds: 550),
                  reverseDuration: const Duration(milliseconds: 480),
                  switchInCurve: const Cubic(0.32, 0.94, 0.6, 1.0),
                  switchOutCurve: const Cubic(0.4, 0.0, 0.2, 1.0),
                  layoutBuilder: (currentChild, previousChildren) => Stack(
                    fit: StackFit.expand,
                    children: [
                      ...previousChildren,
                      if (currentChild != null) currentChild,
                    ],
                  ),
                  transitionBuilder: (child, animation) {
                    final curved = CurvedAnimation(
                      parent: animation,
                      curve: const Cubic(0.32, 0.94, 0.6, 1.0),
                      reverseCurve: const Cubic(0.4, 0.0, 0.2, 1.0),
                    );
                    final scale = Tween<double>(
                      begin: .82,
                      end: 1,
                    ).animate(curved);
                    final opacity = Tween<double>(
                      begin: .94,
                      end: 1,
                    ).animate(curved);
                    return FadeTransition(
                      opacity: opacity,
                      child: ScaleTransition(
                        alignment: _openAnchor,
                        scale: scale,
                        child: child,
                      ),
                    );
                  },
                  child: _activeApp == null
                      ? KeyedSubtree(
                          key: const ValueKey('home'),
                          child: HomeScreen(
                            now: _now,
                            pageController: _homePageController,
                            pages: homePages,
                            dockApps: dockApps,
                            wallpaper: currentWallpaper,
                            pageIndex: _homePage,
                            onPageChanged: (page) =>
                                setState(() => _homePage = page),
                            onOpenApp: (id, anchor) => _openApp(id, anchor),
                            gameLocation: _gameLocation,
                            gameHour: _gameTime,
                          ),
                        )
                      : KeyedSubtree(
                          key: ValueKey('app-$_activeApp'),
                          child: AppWindow(
                            app: _apps.firstWhere(
                              (item) => item.id == _activeApp,
                            ),
                            now: _now,
                            darkMode: _darkMode,
                            wallpaper: currentWallpaper,
                            onClose: _closeApp,
                            onOpenApp: _openApp,
                            onToggleTheme: () =>
                                setState(() => _darkMode = !_darkMode),
                            onChangeWallpaper: () => setState(
                              () => _wallpaperIndex =
                                  (_wallpaperIndex + 1) % _wallpapers.length,
                            ),
                          ),
                        ),
                ),
              ),
              PhoneNavigationBar(
                darkMode: _darkMode || _activeApp == 'camera',
                onRecents: _closeApp,
                onHome: _closePhone,
                onBack: () => unawaited(_handleBack()),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

class DeviceFrame extends StatelessWidget {
  final Widget child;

  const DeviceFrame({super.key, required this.child});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.transparent,
      body: LayoutBuilder(
        builder: (context, constraints) {
          final compactViewport = constraints.maxWidth <= 600;
          final frameScale = compactViewport ? 1.0 : 0.96;
          final rightInset = compactViewport ? 0.0 : 12.0;

          return Align(
            alignment: Alignment.centerRight,
            child: Padding(
              padding: EdgeInsets.only(right: rightInset),
              child: SizedBox(
                width: constraints.maxWidth - rightInset,
                height: constraints.maxHeight * frameScale,
                child: FittedBox(
                  fit: compactViewport ? BoxFit.cover : BoxFit.contain,
                  alignment: compactViewport
                      ? Alignment.topCenter
                      : Alignment.centerRight,
                  child: SizedBox(
                    width: 814,
                    height: 1716,
                    child: Stack(
                      fit: StackFit.expand,
                      children: [
                        Positioned(
                          left: 29,
                          top: 34,
                          width: 750,
                          height: 1604,
                          child: ClipRRect(
                            borderRadius: BorderRadius.circular(40),
                            child: FittedBox(
                              fit: BoxFit.fill,
                              child: SizedBox(
                                width: 380,
                                height: 816,
                                child: child,
                              ),
                            ),
                          ),
                        ),
                        Positioned.fill(
                          child: IgnorePointer(
                            child: Image.asset(
                              'assets/frame.png',
                              fit: BoxFit.fill,
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
              ),
            ),
          );
        },
      ),
    );
  }
}

class AppDefinition {
  final String id;
  final String name;
  final String icon;
  final Color accent;

  const AppDefinition(this.id, this.name, this.icon, this.accent);
}

class HomeScreen extends StatelessWidget {
  final DateTime now;
  final PageController pageController;
  final List<List<AppDefinition>> pages;
  final List<AppDefinition> dockApps;
  final String wallpaper;
  final int pageIndex;
  final ValueChanged<int> onPageChanged;
  final void Function(String id, Alignment anchor) onOpenApp;
  final String gameLocation;
  final String gameHour;

  const HomeScreen({
    super.key,
    required this.now,
    required this.pageController,
    required this.pages,
    required this.dockApps,
    required this.wallpaper,
    required this.pageIndex,
    required this.onPageChanged,
    required this.onOpenApp,
    this.gameLocation = 'San Andreas',
    this.gameHour = '12:00',
  });

  String _time(DateTime value) {
    final hour = value.hour.toString().padLeft(2, '0');
    final minute = value.minute.toString().padLeft(2, '0');
    return '$hour:$minute';
  }

  String _date(DateTime value) {
    const weekdays = [
      'segunda-feira',
      'terça-feira',
      'quarta-feira',
      'quinta-feira',
      'sexta-feira',
      'sábado',
      'domingo',
    ];
    const months = [
      'janeiro',
      'fevereiro',
      'março',
      'abril',
      'maio',
      'junho',
      'julho',
      'agosto',
      'setembro',
      'outubro',
      'novembro',
      'dezembro',
    ];
    return '${weekdays[value.weekday - 1]}, ${value.day} de ${months[value.month - 1]}';
  }

  Widget _page(List<AppDefinition> apps) {
    return Align(
      alignment: Alignment.bottomCenter,
      child: LayoutBuilder(
        builder: (context, constraints) {
          final cellWidth = (constraints.maxWidth - 36 - 27) / 4;
          return Padding(
            padding: const EdgeInsets.fromLTRB(18, 0, 18, 8),
            child: Wrap(
              alignment: WrapAlignment.center,
              spacing: 9,
              runSpacing: 17,
              children: apps
                  .asMap()
                  .entries
                  .map(
                    (entry) => SizedBox(
                      width: cellWidth,
                      child: LauncherIcon(
                        app: entry.value,
                        onTap: () => onOpenApp(
                          entry.value.id,
                          entry.key % 4 >= 2
                              ? Alignment.centerRight
                              : Alignment.centerLeft,
                        ),
                      ),
                    ),
                  )
                  .toList(),
            ),
          );
        },
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Stack(
      fit: StackFit.expand,
      children: [
        Image.asset(wallpaper, fit: BoxFit.cover),
        DecoratedBox(
          decoration: BoxDecoration(
            gradient: LinearGradient(
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter,
              colors: [
                Colors.black.withOpacity(.22),
                Colors.transparent,
                Colors.black.withOpacity(.40),
              ],
              stops: const [0, .43, 1],
            ),
          ),
        ),
        SafeArea(
          bottom: false,
          child: Column(
            children: [
              PhoneStatusBar(now: now, darkMode: true),
              const SizedBox(height: 4),
              Text(
                _date(now),
                style: TextStyle(
                  color: Colors.white.withOpacity(.9),
                  fontSize: 15,
                  fontWeight: FontWeight.w500,
                ),
              ),
              Text(
                _time(now),
                style: const TextStyle(
                  color: Colors.white,
                  fontSize: 64,
                  height: 1.05,
                  fontWeight: FontWeight.w300,
                  letterSpacing: -2,
                ),
              ),
              const SizedBox(height: 12),
              Expanded(
                child: PageView.builder(
                  controller: pageController,
                  itemCount: pages.length,
                  onPageChanged: onPageChanged,
                  itemBuilder: (_, index) => _page(pages[index]),
                ),
              ),
              Padding(
                padding: const EdgeInsets.only(bottom: 8),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: List.generate(
                    pages.length,
                    (index) => GestureDetector(
                      onTap: () => pageController.animateToPage(
                        index,
                        duration: const Duration(milliseconds: 280),
                        curve: Curves.easeOutCubic,
                      ),
                      child: AnimatedContainer(
                        duration: const Duration(milliseconds: 180),
                        width: index == pageIndex ? 8 : 6,
                        height: index == pageIndex ? 8 : 6,
                        margin: const EdgeInsets.symmetric(horizontal: 5),
                        decoration: BoxDecoration(
                          shape: BoxShape.circle,
                          color: index == pageIndex
                              ? Colors.white
                              : Colors.white54,
                        ),
                      ),
                    ),
                  ),
                ),
              ),
              Container(
                margin: const EdgeInsets.fromLTRB(15, 0, 15, 10),
                padding: const EdgeInsets.symmetric(
                  horizontal: 13,
                  vertical: 10,
                ),
                decoration: BoxDecoration(
                  color: Colors.white.withOpacity(.19),
                  borderRadius: BorderRadius.circular(28),
                  border: Border.all(color: Colors.white.withOpacity(.16)),
                ),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceAround,
                  children: dockApps
                      .asMap()
                      .entries
                      .map(
                        (entry) => SmallLauncherIcon(
                          app: entry.value,
                          onTap: () => onOpenApp(
                            entry.value.id,
                            entry.key >= dockApps.length / 2
                                ? Alignment.centerRight
                                : Alignment.centerLeft,
                          ),
                        ),
                      )
                      .toList(),
                ),
              ),
            ],
          ),
        ),
      ],
    );
  }
}

class LauncherIcon extends StatelessWidget {
  final AppDefinition app;
  final VoidCallback onTap;

  const LauncherIcon({super.key, required this.app, required this.onTap});

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 55,
            height: 55,
            decoration: BoxDecoration(
              borderRadius: BorderRadius.circular(16),
              boxShadow: [
                BoxShadow(
                  color: Colors.black.withOpacity(.25),
                  blurRadius: 9,
                  offset: const Offset(0, 4),
                ),
              ],
            ),
            clipBehavior: Clip.antiAlias,
            child: Image.asset(app.icon, fit: BoxFit.cover),
          ),
          const SizedBox(height: 5),
          Text(
            app.name,
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
            style: const TextStyle(
              color: Colors.white,
              fontSize: 11,
              fontWeight: FontWeight.w500,
              shadows: [Shadow(color: Colors.black54, blurRadius: 4)],
            ),
          ),
        ],
      ),
    );
  }
}

class SmallLauncherIcon extends StatelessWidget {
  final AppDefinition app;
  final VoidCallback onTap;

  const SmallLauncherIcon({super.key, required this.app, required this.onTap});

  @override
  Widget build(BuildContext context) => GestureDetector(
    onTap: onTap,
    child: ClipRRect(
      borderRadius: BorderRadius.circular(14),
      child: Image.asset(app.icon, width: 47, height: 47, fit: BoxFit.cover),
    ),
  );
}

class SmallDockButton extends StatelessWidget {
  final VoidCallback onTap;

  const SmallDockButton({super.key, required this.onTap});

  @override
  Widget build(BuildContext context) => GestureDetector(
    onTap: onTap,
    child: Container(
      width: 47,
      height: 47,
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(.72),
        borderRadius: BorderRadius.circular(14),
      ),
      child: const Icon(Icons.apps_rounded, color: Colors.black87, size: 27),
    ),
  );
}

class AppWindow extends StatelessWidget {
  final AppDefinition app;
  final DateTime now;
  final bool darkMode;
  final String wallpaper;
  final VoidCallback onClose;
  final ValueChanged<String> onOpenApp;
  final VoidCallback onToggleTheme;
  final VoidCallback onChangeWallpaper;

  const AppWindow({
    super.key,
    required this.app,
    required this.now,
    required this.darkMode,
    required this.wallpaper,
    required this.onClose,
    required this.onOpenApp,
    required this.onToggleTheme,
    required this.onChangeWallpaper,
  });

  @override
  Widget build(BuildContext context) {
    final cameraShell = app.id == 'camera';
    return Container(
      color: cameraShell
          ? Colors.transparent
          : (darkMode ? const Color(0xFF101114) : const Color(0xFFF7F7F8)),
      child: Column(
        children: [
          Container(
            color: cameraShell ? Colors.black : Colors.transparent,
            child: PhoneStatusBar(now: now, darkMode: cameraShell || darkMode),
          ),
          Expanded(child: _content()),
        ],
      ),
    );
  }

  Widget _content() {
    switch (app.id) {
      case 'whatsapp':
        return WhatsappScreen(darkMode: darkMode);
      case 'instagram':
        return InstagramScreen(darkMode: darkMode);
      case 'bank':
        return BankScreen(darkMode: darkMode);
      case 'uber':
        return UberScreen(darkMode: darkMode);
      case 'camera':
        return CameraScreen();
      case 'phone':
        return PhoneScreen(darkMode: darkMode);
      case 'olx':
        return OlxScreen(darkMode: darkMode);
      case 'settings':
        return SettingsScreen(
          darkMode: darkMode,
          wallpaper: wallpaper,
          onToggleTheme: onToggleTheme,
          onChangeWallpaper: onChangeWallpaper,
        );
      case 'messages':
        return MessagesScreen(darkMode: darkMode);
      case 'gallery':
        return GalleryScreen(darkMode: darkMode);
      case 'calculator':
        return CalculatorScreen(darkMode: darkMode);
      case 'notes':
        return NotesScreen(darkMode: darkMode);
      default:
        return GenericAppScreen(app: app, darkMode: darkMode);
    }
  }
}

String _phoneStatusTime(DateTime value) {
  final hour = value.hour.toString().padLeft(2, '0');
  final minute = value.minute.toString().padLeft(2, '0');
  return '$hour:$minute';
}

class PhoneStatusBar extends StatelessWidget {
  final DateTime now;
  final bool darkMode;

  const PhoneStatusBar({super.key, required this.now, required this.darkMode});

  @override
  Widget build(BuildContext context) {
    final foreground = darkMode ? Colors.white : Colors.black87;

    return Padding(
      padding: const EdgeInsets.fromLTRB(20, 7, 16, 1),
      child: SizedBox(
        height: 20,
        child: Row(
          children: [
            Text(
              _phoneStatusTime(now),
              style: TextStyle(
                color: foreground,
                fontSize: 13,
                fontWeight: FontWeight.w700,
              ),
            ),
            const Spacer(),
            Icon(
              Icons.signal_cellular_alt_rounded,
              color: foreground,
              size: 15,
            ),
            const SizedBox(width: 5),
            Icon(Icons.wifi_rounded, color: foreground, size: 15),
            const SizedBox(width: 6),
            Text(
              '100%',
              style: TextStyle(
                color: foreground,
                fontSize: 12,
                fontWeight: FontWeight.w600,
              ),
            ),
            const SizedBox(width: 2),
            Icon(Icons.battery_full_rounded, color: foreground, size: 17),
          ],
        ),
      ),
    );
  }
}

class PhoneNavigationBar extends StatelessWidget {
  final bool darkMode;
  final VoidCallback onRecents;
  final VoidCallback onHome;
  final VoidCallback onBack;

  const PhoneNavigationBar({
    super.key,
    required this.darkMode,
    required this.onRecents,
    required this.onHome,
    required this.onBack,
  });

  @override
  Widget build(BuildContext context) {
    final foreground = darkMode ? Colors.white : Colors.black87;
    return Container(
      height: 42,
      color: darkMode ? const Color(0xFF101114) : Colors.white,
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceAround,
        children: [
          IconButton(
            onPressed: onRecents,
            color: foreground,
            padding: EdgeInsets.zero,
            icon: const _RecentAppsIcon(),
          ),
          IconButton(
            onPressed: onHome,
            color: foreground,
            iconSize: 24,
            padding: EdgeInsets.zero,
            icon: const Icon(Icons.crop_square_rounded),
          ),
          IconButton(
            onPressed: onBack,
            color: foreground,
            iconSize: 29,
            padding: EdgeInsets.zero,
            icon: const Icon(Icons.chevron_left_rounded),
          ),
        ],
      ),
    );
  }
}

class _RecentAppsIcon extends StatelessWidget {
  const _RecentAppsIcon();

  @override
  Widget build(BuildContext context) {
    final color = IconTheme.of(context).color ?? Colors.white;
    return SizedBox(
      width: 24,
      height: 24,
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        crossAxisAlignment: CrossAxisAlignment.center,
        children: [
          _bar(color, 13),
          const SizedBox(width: 3),
          _bar(color, 18),
          const SizedBox(width: 3),
          _bar(color, 13),
        ],
      ),
    );
  }

  Widget _bar(Color color, double height) {
    return Container(
      width: 2.5,
      height: height,
      decoration: BoxDecoration(
        color: color,
        borderRadius: BorderRadius.circular(3),
      ),
    );
  }
}

class AppTopBar extends StatelessWidget {
  final AppDefinition app;
  final VoidCallback onClose;

  const AppTopBar({super.key, required this.app, required this.onClose});

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(10, 5, 16, 9),
      child: Row(
        children: [
          IconButton(
            onPressed: onClose,
            icon: const Icon(Icons.arrow_back_rounded),
          ),
          ClipRRect(
            borderRadius: BorderRadius.circular(10),
            child: Image.asset(
              app.icon,
              width: 34,
              height: 34,
              fit: BoxFit.cover,
            ),
          ),
          const SizedBox(width: 10),
          Text(
            app.name,
            style: const TextStyle(fontSize: 19, fontWeight: FontWeight.w700),
          ),
          const Spacer(),
          IconButton(
            onPressed: () {},
            icon: const Icon(Icons.more_vert_rounded),
          ),
        ],
      ),
    );
  }
}

class WhatsappScreen extends StatefulWidget {
  final bool darkMode;

  const WhatsappScreen({super.key, required this.darkMode});

  @override
  State<WhatsappScreen> createState() => _WhatsappScreenState();
}

class _WhatsappChat {
  final String name;
  final String preview;
  final String time;
  final String initials;
  final Color avatarColor;

  const _WhatsappChat(
    this.name,
    this.preview,
    this.time,
    this.initials,
    this.avatarColor,
  );
}

class _WhatsappMessage {
  final String text;
  final String time;
  final bool fromMe;
  final bool audio;
  final String? audioPath;

  const _WhatsappMessage(
    this.text,
    this.time,
    this.fromMe, {
    this.audio = false,
    this.audioPath,
  });
}

class _WhatsappScreenState extends State<WhatsappScreen> {
  int tab = 0;
  String query = '';
  _WhatsappChat? activeChat;

  final chats = const [
    _WhatsappChat('Teste', 'Testeee3333333s', '18:53', 'T', Color(0xFFE4E4E4)),
  ];

  @override
  Widget build(BuildContext context) {
    if (activeChat != null) {
      return _WhatsappChatScreen(
        chat: activeChat!,
        darkMode: widget.darkMode,
        onBack: () => setState(() => activeChat = null),
      );
    }

    final dark = widget.darkMode;
    final background = dark ? const Color(0xFF0B141A) : Colors.white;
    final text = dark ? Colors.white : const Color(0xFF17212B);
    final visibleChats = chats
        .where((chat) => chat.name.toLowerCase().contains(query.toLowerCase()))
        .toList();

    return Container(
      color: background,
      child: Column(
        children: [
          Padding(
            padding: const EdgeInsets.fromLTRB(16, 11, 10, 5),
            child: Row(
              children: [
                Text(
                  'WhatsApp',
                  style: TextStyle(
                    color: text,
                    fontSize: 23,
                    fontWeight: FontWeight.w500,
                  ),
                ),
                const Spacer(),
                IconButton(
                  onPressed: () {},
                  icon: Icon(Icons.camera_alt_outlined, color: text),
                ),
                IconButton(
                  onPressed: () {},
                  icon: Icon(Icons.more_vert, color: text),
                ),
              ],
            ),
          ),
          Padding(
            padding: const EdgeInsets.fromLTRB(14, 0, 14, 10),
            child: TextField(
              onChanged: (value) => setState(() => query = value),
              style: TextStyle(color: text),
              decoration: InputDecoration(
                hintText: 'Pergunte à Meta AI ou pesquise',
                hintStyle: TextStyle(color: text.withOpacity(.46)),
                prefixIcon: Icon(Icons.search, color: text.withOpacity(.55)),
                filled: true,
                fillColor: dark
                    ? const Color(0xFF20272B)
                    : const Color(0xFFF0F2F5),
                contentPadding: const EdgeInsets.symmetric(vertical: 0),
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(24),
                  borderSide: BorderSide.none,
                ),
              ),
            ),
          ),
          Expanded(
            child: tab == 0
                ? ListView.separated(
                    padding: const EdgeInsets.only(top: 1),
                    itemCount: visibleChats.length,
                    separatorBuilder: (_, __) => Divider(
                      height: 1,
                      indent: 76,
                      color: text.withOpacity(.08),
                    ),
                    itemBuilder: (_, index) {
                      final chat = visibleChats[index];
                      return ListTile(
                        onTap: () => setState(() => activeChat = chat),
                        contentPadding: const EdgeInsets.symmetric(
                          horizontal: 16,
                          vertical: 3,
                        ),
                        leading: _WhatsappAvatar(
                          initials: chat.initials,
                          background: chat.avatarColor,
                          darkMode: dark,
                        ),
                        title: Text(
                          chat.name,
                          style: TextStyle(
                            color: text,
                            fontSize: 16,
                            fontWeight: FontWeight.w600,
                          ),
                        ),
                        subtitle: Row(
                          children: [
                            Icon(
                              Icons.done_all,
                              color: text.withOpacity(.52),
                              size: 15,
                            ),
                            const SizedBox(width: 4),
                            Expanded(
                              child: Text(
                                chat.preview,
                                maxLines: 1,
                                overflow: TextOverflow.ellipsis,
                                style: TextStyle(color: text.withOpacity(.55)),
                              ),
                            ),
                          ],
                        ),
                        trailing: Text(
                          chat.time,
                          style: TextStyle(
                            color: text.withOpacity(.48),
                            fontSize: 12,
                          ),
                        ),
                      );
                    },
                  )
                : _WhatsappEmptyTab(tab: tab, text: text, darkMode: dark),
          ),
          _WhatsappTabs(
            selected: tab,
            darkMode: dark,
            onSelected: (index) => setState(() => tab = index),
          ),
        ],
      ),
    );
  }
}

class _WhatsappAvatar extends StatelessWidget {
  final String initials;
  final Color background;
  final bool darkMode;

  const _WhatsappAvatar({
    required this.initials,
    required this.background,
    required this.darkMode,
  });

  @override
  Widget build(BuildContext context) {
    return CircleAvatar(
      radius: 25,
      backgroundColor: background,
      child: Icon(
        Icons.person,
        color: darkMode ? const Color(0xFFB5B9BB) : Colors.white,
        size: 29,
      ),
    );
  }
}

class _WhatsappEmptyTab extends StatelessWidget {
  final int tab;
  final Color text;
  final bool darkMode;

  const _WhatsappEmptyTab({
    required this.tab,
    required this.text,
    required this.darkMode,
  });

  @override
  Widget build(BuildContext context) {
    final labels = [
      'Compartilhe atualizações de status',
      'Organize seus grupos e comunidades',
      'Nenhuma chamada recente',
    ];
    final icons = [
      Icons.camera_alt_outlined,
      Icons.groups_outlined,
      Icons.call_outlined,
    ];
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(icons[tab - 1], size: 42, color: text.withOpacity(.22)),
          const SizedBox(height: 12),
          Text(labels[tab - 1], style: TextStyle(color: text.withOpacity(.6))),
          if (tab == 1) ...[
            const SizedBox(height: 16),
            FilledButton(
              onPressed: () {},
              style: FilledButton.styleFrom(
                backgroundColor: const Color(0xFF25D366),
              ),
              child: const Text('Criar atualização'),
            ),
          ],
        ],
      ),
    );
  }
}

class _WhatsappTabs extends StatelessWidget {
  final int selected;
  final bool darkMode;
  final ValueChanged<int> onSelected;

  const _WhatsappTabs({
    required this.selected,
    required this.darkMode,
    required this.onSelected,
  });

  @override
  Widget build(BuildContext context) {
    const labels = ['Conversas', 'Atualizações', 'Comunidades', 'Ligações'];
    const icons = [
      Icons.chat_bubble_rounded,
      Icons.circle_outlined,
      Icons.groups_outlined,
      Icons.call_outlined,
    ];
    final text = darkMode ? Colors.white : const Color(0xFF17212B);
    return Container(
      height: 67,
      decoration: BoxDecoration(
        color: darkMode ? const Color(0xFF0B141A) : Colors.white,
        border: Border(top: BorderSide(color: text.withOpacity(.08))),
      ),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceAround,
        children: List.generate(
          labels.length,
          (index) => GestureDetector(
            onTap: () => onSelected(index),
            child: SizedBox(
              width: 74,
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  AnimatedContainer(
                    duration: const Duration(milliseconds: 180),
                    width: 47,
                    height: 30,
                    decoration: BoxDecoration(
                      color: selected == index
                          ? const Color(0xFF0B5E4B)
                          : Colors.transparent,
                      borderRadius: BorderRadius.circular(18),
                    ),
                    child: Icon(
                      icons[index],
                      color: selected == index
                          ? Colors.white
                          : text.withOpacity(.78),
                      size: 20,
                    ),
                  ),
                  const SizedBox(height: 2),
                  Text(
                    labels[index],
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                    style: TextStyle(
                      color: text,
                      fontSize: 11,
                      fontWeight: selected == index
                          ? FontWeight.w700
                          : FontWeight.w500,
                    ),
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }
}

class _WhatsappChatScreen extends StatefulWidget {
  final _WhatsappChat chat;
  final bool darkMode;
  final VoidCallback onBack;

  const _WhatsappChatScreen({
    required this.chat,
    required this.darkMode,
    required this.onBack,
  });

  @override
  State<_WhatsappChatScreen> createState() => _WhatsappChatScreenState();
}

class _WhatsappChatScreenState extends State<_WhatsappChatScreen> {
  final controller = TextEditingController();
  final AudioRecorder _recorder = AudioRecorder();
  final AudioPlayer _audioPlayer = AudioPlayer();
  Timer? _recordTimer;
  int _recordSeconds = 0;
  bool isRecording = false;
  bool showAttachments = false;
  bool _cancelRecording = false;
  bool _recordingStarted = false;
  bool _holdingToRecord = false;
  String? _playingAudioPath;
  final messages = <_WhatsappMessage>[
    const _WhatsappMessage('Oi, teste', '19:17', true),
    const _WhatsappMessage('Testeee', '19:18', false),
  ];

  @override
  void initState() {
    super.initState();
    _audioPlayer.onPlayerComplete.listen((_) {
      if (mounted) setState(() => _playingAudioPath = null);
    });
  }

  @override
  void dispose() {
    _recordTimer?.cancel();
    unawaited(_recorder.dispose());
    unawaited(_audioPlayer.dispose());
    controller.dispose();
    super.dispose();
  }

  String _recordDuration() {
    final minutes = (_recordSeconds ~/ 60).toString().padLeft(1, '0');
    final seconds = (_recordSeconds % 60).toString().padLeft(2, '0');
    return '$minutes:$seconds';
  }

  Future<void> _startRecording({bool holdGesture = false}) async {
    _holdingToRecord = holdGesture;
    try {
      final permission = await _recorder.hasPermission();
      if (!permission || !mounted) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('Permissão de microfone negada')),
          );
        }
        return;
      }

      final directory = await getTemporaryDirectory();
      final path =
          '${directory.path}/whatsapp_${DateTime.now().millisecondsSinceEpoch}.m4a';
      await _recorder.start(
        const RecordConfig(
          encoder: AudioEncoder.aacLc,
          sampleRate: 44100,
          numChannels: 1,
          autoGain: true,
          echoCancel: true,
          noiseSuppress: true,
        ),
        path: path,
      );
      if (!mounted) return;
      if (holdGesture && !_holdingToRecord) {
        await _recorder.cancel();
        return;
      }
      setState(() {
        showAttachments = false;
        isRecording = true;
        _recordingStarted = true;
        _recordSeconds = 0;
        _cancelRecording = false;
      });
      _recordTimer = Timer.periodic(const Duration(seconds: 1), (_) {
        if (!mounted || !isRecording) return;
        setState(() => _recordSeconds++);
      });
    } catch (_) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Não foi possível iniciar o microfone')),
        );
      }
    }
  }

  Future<void> _stopRecording({bool cancel = false}) async {
    if (!isRecording && !_recordingStarted) return;
    _recordTimer?.cancel();
    final duration = _recordDuration();
    String? path;
    try {
      if (cancel) {
        await _recorder.cancel();
      } else {
        path = await _recorder.stop();
      }
    } catch (_) {
      path = null;
    }
    if (!mounted) return;
    setState(() {
      isRecording = false;
      _recordingStarted = false;
      _holdingToRecord = false;
      _cancelRecording = false;
    });
    if (!cancel && path != null && _recordSeconds > 0) {
      setState(() {
        messages.add(
          _WhatsappMessage(
            'Áudio • $duration',
            '19:20',
            true,
            audio: true,
            audioPath: path,
          ),
        );
      });
    }
  }

  void _toggleRecording() {
    if (isRecording) {
      unawaited(_stopRecording());
    } else {
      unawaited(_startRecording());
    }
  }

  void _cancelCurrentRecording() {
    _cancelRecording = true;
    unawaited(_stopRecording(cancel: true));
  }

  Future<void> _playAudio(String path) async {
    if (_playingAudioPath == path) {
      await _audioPlayer.stop();
      if (mounted) setState(() => _playingAudioPath = null);
      return;
    }
    await _audioPlayer.play(DeviceFileSource(path));
    if (mounted) setState(() => _playingAudioPath = path);
  }

  void _send() {
    final text = controller.text.trim();
    if (text.isEmpty) return;
    setState(() {
      messages.add(_WhatsappMessage(text, '19:19', true));
      controller.clear();
      showAttachments = false;
    });
  }

  @override
  Widget build(BuildContext context) {
    final dark = widget.darkMode;
    final background = dark ? const Color(0xFF0B141A) : const Color(0xFFECE5DD);
    final text = dark ? Colors.white : const Color(0xFF17212B);

    return Container(
      color: background,
      child: Stack(
        fit: StackFit.expand,
        children: [
          Positioned.fill(
            child: Image.asset(
              'assets/apps/whatsapp/night_background_chat.jpg',
              fit: BoxFit.cover,
              opacity: AlwaysStoppedAnimation(dark ? .18 : .08),
            ),
          ),
          Positioned.fill(
            child: Container(
              color: dark ? const Color(0xD90B141A) : const Color(0xD9ECE5DD),
            ),
          ),
          Column(
            children: [
              Container(
                padding: const EdgeInsets.fromLTRB(5, 4, 7, 7),
                decoration: BoxDecoration(
                  color: dark ? const Color(0xFF111B21) : Colors.white,
                  border: Border(
                    bottom: BorderSide(color: text.withOpacity(.08)),
                  ),
                ),
                child: Row(
                  children: [
                    IconButton(
                      onPressed: widget.onBack,
                      icon: Icon(Icons.arrow_back, color: text),
                    ),
                    _WhatsappAvatar(
                      initials: widget.chat.initials,
                      background: widget.chat.avatarColor,
                      darkMode: dark,
                    ),
                    const SizedBox(width: 9),
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            widget.chat.name,
                            style: TextStyle(
                              color: text,
                              fontSize: 16,
                              fontWeight: FontWeight.w700,
                            ),
                          ),
                          Text(
                            'online',
                            style: TextStyle(
                              color: text.withOpacity(.54),
                              fontSize: 11,
                            ),
                          ),
                        ],
                      ),
                    ),
                    IconButton(
                      onPressed: () {},
                      icon: Icon(Icons.call_outlined, color: text),
                    ),
                    IconButton(
                      onPressed: () {},
                      icon: Icon(Icons.more_vert, color: text),
                    ),
                  ],
                ),
              ),
              Expanded(
                child: ListView.builder(
                  padding: const EdgeInsets.fromLTRB(11, 14, 11, 8),
                  itemCount: messages.length,
                  itemBuilder: (_, index) {
                    final message = messages[index];
                    return Align(
                      alignment: message.fromMe
                          ? Alignment.centerRight
                          : Alignment.centerLeft,
                      child: Container(
                        constraints: const BoxConstraints(maxWidth: 285),
                        margin: const EdgeInsets.only(bottom: 8),
                        padding: const EdgeInsets.fromLTRB(12, 8, 9, 6),
                        decoration: BoxDecoration(
                          color: message.fromMe
                              ? (dark
                                    ? const Color(0xFF005C4B)
                                    : const Color(0xFFD9FDD3))
                              : (dark ? const Color(0xFF202C33) : Colors.white),
                          borderRadius: BorderRadius.only(
                            topLeft: const Radius.circular(14),
                            topRight: const Radius.circular(14),
                            bottomLeft: Radius.circular(
                              message.fromMe ? 14 : 3,
                            ),
                            bottomRight: Radius.circular(
                              message.fromMe ? 3 : 14,
                            ),
                          ),
                        ),
                        child: Row(
                          mainAxisSize: MainAxisSize.min,
                          crossAxisAlignment: CrossAxisAlignment.end,
                          children: [
                            if (message.audio) ...[
                              IconButton(
                                onPressed: message.audioPath == null
                                    ? null
                                    : () => _playAudio(message.audioPath!),
                                padding: EdgeInsets.zero,
                                constraints: const BoxConstraints(),
                                icon: Icon(
                                  _playingAudioPath == message.audioPath
                                      ? Icons.pause_rounded
                                      : Icons.play_arrow_rounded,
                                  color: text,
                                  size: 22,
                                ),
                              ),
                              const SizedBox(width: 5),
                              SizedBox(
                                width: 92,
                                child: _AudioWaveform(
                                  color: text.withOpacity(.72),
                                  height: 24,
                                ),
                              ),
                              const SizedBox(width: 5),
                            ] else
                              Flexible(
                                child: Text(
                                  message.text,
                                  style: TextStyle(color: text, fontSize: 14),
                                ),
                              ),
                            const SizedBox(width: 8),
                            Text(
                              message.time,
                              style: TextStyle(
                                color: text.withOpacity(.52),
                                fontSize: 10,
                              ),
                            ),
                            if (message.fromMe) ...[
                              const SizedBox(width: 3),
                              Icon(
                                Icons.done_all,
                                size: 14,
                                color: dark
                                    ? const Color(0xFF8ED7CA)
                                    : const Color(0xFF4C9A89),
                              ),
                            ],
                          ],
                        ),
                      ),
                    );
                  },
                ),
              ),
              if (isRecording)
                Container(
                  padding: const EdgeInsets.fromLTRB(12, 7, 10, 5),
                  color: dark ? const Color(0xFF111B21) : Colors.white,
                  child: Row(
                    children: [
                      const Icon(Icons.mic, color: Colors.red, size: 18),
                      const SizedBox(width: 7),
                      Text(
                        _recordDuration(),
                        style: TextStyle(color: text, fontSize: 13),
                      ),
                      const SizedBox(width: 7),
                      Expanded(
                        child: _AudioWaveform(
                          color: text.withOpacity(.55),
                          height: 24,
                        ),
                      ),
                      const SizedBox(width: 7),
                      Icon(
                        _cancelRecording
                            ? Icons.delete_outline
                            : Icons.chevron_left,
                        color: _cancelRecording
                            ? Colors.red
                            : text.withOpacity(.55),
                        size: 18,
                      ),
                      const SizedBox(width: 3),
                      Text(
                        _cancelRecording
                            ? 'Solte para cancelar'
                            : 'Deslize para cancelar',
                        style: TextStyle(
                          color: _cancelRecording
                              ? Colors.red
                              : text.withOpacity(.52),
                          fontSize: 10,
                        ),
                      ),
                    ],
                  ),
                ),
              AnimatedContainer(
                duration: const Duration(milliseconds: 220),
                height: showAttachments ? 82 : 0,
                clipBehavior: Clip.hardEdge,
                decoration: BoxDecoration(
                  color: dark ? const Color(0xFF111B21) : Colors.white,
                ),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceAround,
                  children: [
                    _WhatsappAttachment(
                      icon: Icons.photo_outlined,
                      label: 'Galeria',
                      color: const Color(0xFF4087E8),
                    ),
                    _WhatsappAttachment(
                      icon: Icons.camera_alt_outlined,
                      label: 'Câmera',
                      color: const Color(0xFFE91E63),
                    ),
                    _WhatsappAttachment(
                      icon: Icons.location_on_outlined,
                      label: 'Localização',
                      color: const Color(0xFF00A896),
                    ),
                    _WhatsappAttachment(
                      icon: Icons.person_outline,
                      label: 'Contatos',
                      color: const Color(0xFF2196F3),
                    ),
                  ],
                ),
              ),
              Padding(
                padding: const EdgeInsets.fromLTRB(9, 7, 9, 8),
                child: Row(
                  children: [
                    Expanded(
                      child: Container(
                        decoration: BoxDecoration(
                          color: dark ? const Color(0xFF202C33) : Colors.white,
                          borderRadius: BorderRadius.circular(22),
                        ),
                        child: Row(
                          children: [
                            IconButton(
                              onPressed: () => setState(
                                () => showAttachments = !showAttachments,
                              ),
                              icon: Icon(
                                Icons.add_circle_outline,
                                color: text.withOpacity(.62),
                              ),
                            ),
                            Expanded(
                              child: TextField(
                                controller: controller,
                                onChanged: (_) => setState(() {}),
                                onSubmitted: (_) => _send(),
                                style: TextStyle(color: text),
                                decoration: InputDecoration(
                                  hintText: 'Mensagem',
                                  hintStyle: TextStyle(
                                    color: text.withOpacity(.42),
                                  ),
                                  border: InputBorder.none,
                                ),
                              ),
                            ),
                            IconButton(
                              onPressed: () => setState(
                                () => showAttachments = !showAttachments,
                              ),
                              icon: Icon(
                                Icons.attach_file,
                                color: text.withOpacity(.62),
                              ),
                            ),
                          ],
                        ),
                      ),
                    ),
                    const SizedBox(width: 7),
                    GestureDetector(
                      onTap: controller.text.trim().isEmpty
                          ? _toggleRecording
                          : _send,
                      onLongPressStart: controller.text.trim().isEmpty
                          ? (_) => unawaited(_startRecording(holdGesture: true))
                          : null,
                      onLongPressMoveUpdate: controller.text.trim().isEmpty
                          ? (details) {
                              if (!isRecording) return;
                              if (details.offsetFromOrigin.dx < -48 &&
                                  !_cancelRecording) {
                                setState(() => _cancelRecording = true);
                              } else if (details.offsetFromOrigin.dx >= -48 &&
                                  _cancelRecording) {
                                setState(() => _cancelRecording = false);
                              }
                            }
                          : null,
                      onLongPressEnd: controller.text.trim().isEmpty
                          ? (_) {
                              _holdingToRecord = false;
                              if (isRecording) {
                                unawaited(
                                  _stopRecording(cancel: _cancelRecording),
                                );
                              }
                            }
                          : null,
                      child: Container(
                        width: 52,
                        height: 52,
                        decoration: const BoxDecoration(
                          color: Color(0xFF25D366),
                          shape: BoxShape.circle,
                        ),
                        child: Icon(
                          isRecording
                              ? Icons.stop_rounded
                              : (controller.text.trim().isEmpty
                                    ? Icons.mic_none
                                    : Icons.send),
                          color: Colors.white,
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }
}

class _AudioWaveform extends StatelessWidget {
  final Color color;
  final double height;

  const _AudioWaveform({required this.color, required this.height});

  @override
  Widget build(BuildContext context) {
    const bars = [
      0.30,
      0.58,
      0.42,
      0.78,
      0.52,
      0.90,
      0.36,
      0.66,
      0.48,
      0.82,
      0.56,
      0.72,
      0.34,
      0.62,
      0.46,
      0.86,
      0.42,
      0.68,
    ];
    return SizedBox(
      height: height,
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        crossAxisAlignment: CrossAxisAlignment.center,
        children: bars
            .map(
              (value) => Container(
                width: 2,
                height: height * value,
                decoration: BoxDecoration(
                  color: color,
                  borderRadius: BorderRadius.circular(3),
                ),
              ),
            )
            .toList(),
      ),
    );
  }
}

class _WhatsappAttachment extends StatelessWidget {
  final IconData icon;
  final String label;
  final Color color;

  const _WhatsappAttachment({
    required this.icon,
    required this.label,
    required this.color,
  });

  @override
  Widget build(BuildContext context) {
    return Column(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        Container(
          width: 42,
          height: 42,
          decoration: BoxDecoration(
            color: Colors.black.withOpacity(.25),
            borderRadius: BorderRadius.circular(10),
            border: Border.all(color: Colors.white.withOpacity(.12)),
          ),
          child: Icon(icon, color: color, size: 23),
        ),
        const SizedBox(height: 3),
        Text(
          label,
          style: const TextStyle(color: Colors.white70, fontSize: 10),
        ),
      ],
    );
  }
}

class InstagramScreen extends StatefulWidget {
  final bool darkMode;

  const InstagramScreen({super.key, required this.darkMode});

  @override
  State<InstagramScreen> createState() => _InstagramScreenState();
}

class _InstagramScreenState extends State<InstagramScreen> {
  bool liked = false;
  bool profile = false;
  int profileTab = 0;

  @override
  Widget build(BuildContext context) {
    if (profile) {
      return InstagramProfileScreen(
        darkMode: widget.darkMode,
        selectedTab: profileTab,
        onHome: () => setState(() => profile = false),
        onProfile: () => setState(() => profile = true),
        onTabChanged: (tab) => setState(() => profileTab = tab),
      );
    }

    return InstagramFeedScreen(
      darkMode: widget.darkMode,
      liked: liked,
      onLike: () => setState(() => liked = !liked),
      onProfile: () => setState(() => profile = true),
    );
  }
}

class InstagramFeedScreen extends StatelessWidget {
  final bool darkMode;
  final bool liked;
  final VoidCallback onLike;
  final VoidCallback onProfile;

  const InstagramFeedScreen({
    super.key,
    required this.darkMode,
    required this.liked,
    required this.onLike,
    required this.onProfile,
  });

  @override
  Widget build(BuildContext context) {
    final text = darkMode ? Colors.white : const Color(0xFF111111);
    final background = darkMode ? const Color(0xFF101114) : Colors.white;

    return Container(
      color: background,
      child: Column(
        children: [
          Padding(
            padding: const EdgeInsets.fromLTRB(16, 4, 10, 2),
            child: Row(
              children: [
                Image.asset(
                  'assets/apps/instagram/logo.png',
                  width: 112,
                  height: 38,
                  fit: BoxFit.contain,
                  alignment: Alignment.centerLeft,
                ),
                const Spacer(),
                IconButton(
                  onPressed: () {},
                  icon: Icon(Icons.favorite_border, color: text, size: 25),
                ),
                IconButton(
                  onPressed: () {},
                  icon: Icon(Icons.send_outlined, color: text, size: 24),
                ),
              ],
            ),
          ),
          SizedBox(
            height: 91,
            child: ListView(
              scrollDirection: Axis.horizontal,
              padding: const EdgeInsets.symmetric(horizontal: 12),
              children: const [
                StoryBubble(label: 'Seu Story', isAdd: true),
                StoryBubble(label: 'kalkcs'),
                StoryBubble(label: 'Mariana'),
                StoryBubble(label: 'Lucas'),
                StoryBubble(label: 'Ana'),
              ],
            ),
          ),
          Divider(height: 1, color: text.withOpacity(.12)),
          Expanded(
            child: Column(
              children: [
                GestureDetector(
                  onTap: onProfile,
                  child: Padding(
                    padding: const EdgeInsets.fromLTRB(12, 8, 12, 6),
                    child: Row(
                      children: [
                        const CircleAvatar(
                          radius: 17,
                          backgroundImage: AssetImage(
                            'assets/apps/instagram/avatar.png',
                          ),
                        ),
                        const SizedBox(width: 9),
                        Text(
                          'kalkcs',
                          style: TextStyle(
                            color: text,
                            fontSize: 13,
                            fontWeight: FontWeight.w700,
                          ),
                        ),
                        const SizedBox(width: 4),
                        Image.asset(
                          'assets/apps/instagram/verified.png',
                          width: 15,
                          height: 15,
                        ),
                        const Spacer(),
                        Icon(Icons.more_horiz, color: text),
                      ],
                    ),
                  ),
                ),
                Expanded(
                  child: Container(
                    color: darkMode ? const Color(0xFF101114) : Colors.white,
                  ),
                ),
                Padding(
                  padding: const EdgeInsets.fromLTRB(12, 5, 12, 0),
                  child: Row(
                    children: [
                      IconButton(
                        padding: EdgeInsets.zero,
                        constraints: const BoxConstraints(),
                        onPressed: onLike,
                        icon: Icon(
                          liked ? Icons.favorite : Icons.favorite_border,
                          color: liked ? Colors.red : text,
                          size: 28,
                        ),
                      ),
                      const SizedBox(width: 13),
                      Icon(Icons.chat_bubble_outline, color: text, size: 26),
                      const SizedBox(width: 13),
                      Icon(Icons.send_outlined, color: text, size: 25),
                      const Spacer(),
                      Icon(Icons.bookmark_border, color: text, size: 27),
                    ],
                  ),
                ),
                Padding(
                  padding: const EdgeInsets.fromLTRB(12, 3, 12, 2),
                  child: Row(
                    children: [
                      Icon(Icons.favorite_border, color: text, size: 17),
                      const SizedBox(width: 5),
                      Text('0', style: TextStyle(color: text, fontSize: 13)),
                      const SizedBox(width: 14),
                      Icon(Icons.chat_bubble_outline, color: text, size: 17),
                      const SizedBox(width: 5),
                      Text('1', style: TextStyle(color: text, fontSize: 13)),
                    ],
                  ),
                ),
                Align(
                  alignment: Alignment.centerLeft,
                  child: Padding(
                    padding: const EdgeInsets.fromLTRB(12, 0, 12, 7),
                    child: Text(
                      'Ver curtidas\nkalkcs teste',
                      style: TextStyle(color: text, fontSize: 13),
                    ),
                  ),
                ),
              ],
            ),
          ),
          InstagramBottomNav(
            darkMode: darkMode,
            selected: 0,
            onHome: () {},
            onProfile: onProfile,
          ),
        ],
      ),
    );
  }
}

class InstagramProfileScreen extends StatelessWidget {
  final bool darkMode;
  final int selectedTab;
  final VoidCallback onHome;
  final VoidCallback onProfile;
  final ValueChanged<int> onTabChanged;

  const InstagramProfileScreen({
    super.key,
    required this.darkMode,
    required this.selectedTab,
    required this.onHome,
    required this.onProfile,
    required this.onTabChanged,
  });

  @override
  Widget build(BuildContext context) {
    final text = darkMode ? Colors.white : const Color(0xFF111111);
    final background = darkMode ? const Color(0xFF101114) : Colors.white;

    return Container(
      color: background,
      child: Column(
        children: [
          Padding(
            padding: const EdgeInsets.fromLTRB(16, 8, 14, 2),
            child: Row(
              children: [
                Text(
                  'kalkcs',
                  style: TextStyle(
                    color: text,
                    fontSize: 21,
                    fontWeight: FontWeight.w700,
                  ),
                ),
                const SizedBox(width: 4),
                Image.asset(
                  'assets/apps/instagram/verified.png',
                  width: 17,
                  height: 17,
                ),
                const Spacer(),
                Icon(Icons.menu, color: text, size: 26),
              ],
            ),
          ),
          Padding(
            padding: const EdgeInsets.fromLTRB(16, 10, 16, 4),
            child: Row(
              children: [
                const CircleAvatar(
                  radius: 36,
                  backgroundColor: Color(0xFFD7D7D7),
                  backgroundImage: AssetImage(
                    'assets/apps/instagram/avatar.png',
                  ),
                ),
                const SizedBox(width: 20),
                Expanded(
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.spaceAround,
                    children: const [
                      InstagramStat(value: '0', label: 'publicações'),
                      InstagramStat(value: '0', label: 'seguidores'),
                      InstagramStat(value: '0', label: 'seguindo'),
                    ],
                  ),
                ),
              ],
            ),
          ),
          Align(
            alignment: Alignment.centerLeft,
            child: Padding(
              padding: const EdgeInsets.fromLTRB(18, 2, 18, 0),
              child: Text(
                'kalkcs\n????????',
                style: TextStyle(color: text, fontSize: 14, height: 1.45),
              ),
            ),
          ),
          Padding(
            padding: const EdgeInsets.fromLTRB(18, 9, 18, 7),
            child: SizedBox(
              width: double.infinity,
              height: 34,
              child: FilledButton(
                onPressed: () {},
                style: FilledButton.styleFrom(
                  backgroundColor: darkMode
                      ? const Color(0xFF373737)
                      : const Color(0xFFD0D0D0),
                  foregroundColor: text,
                  shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(7),
                  ),
                ),
                child: const Text('Editar perfil'),
              ),
            ),
          ),
          Align(
            alignment: Alignment.centerLeft,
            child: Padding(
              padding: const EdgeInsets.only(left: 18, top: 2, bottom: 2),
              child: Column(
                children: [
                  Container(
                    width: 68,
                    height: 68,
                    decoration: BoxDecoration(
                      shape: BoxShape.circle,
                      border: Border.all(color: text, width: 1.5),
                    ),
                    child: Icon(Icons.add, color: text, size: 30),
                  ),
                  const SizedBox(height: 3),
                  Text('Novo', style: TextStyle(color: text, fontSize: 11)),
                ],
              ),
            ),
          ),
          const SizedBox(height: 7),
          Row(
            children: [
              Expanded(
                child: GestureDetector(
                  onTap: () => onTabChanged(0),
                  child: Container(
                    height: 43,
                    decoration: BoxDecoration(
                      border: Border(
                        bottom: BorderSide(
                          color: selectedTab == 0 ? text : Colors.transparent,
                          width: 2,
                        ),
                      ),
                    ),
                    child: Icon(Icons.grid_on, color: text, size: 24),
                  ),
                ),
              ),
              Expanded(
                child: GestureDetector(
                  onTap: () => onTabChanged(1),
                  child: Container(
                    height: 43,
                    decoration: BoxDecoration(
                      border: Border(
                        bottom: BorderSide(
                          color: selectedTab == 1 ? text : Colors.transparent,
                          width: 2,
                        ),
                      ),
                    ),
                    child: Icon(
                      Icons.account_box_outlined,
                      color: text,
                      size: 25,
                    ),
                  ),
                ),
              ),
            ],
          ),
          Expanded(
            child: Center(
              child: selectedTab == 0
                  ? Column(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        Icon(Icons.camera_alt_outlined, color: text, size: 58),
                        const SizedBox(height: 10),
                        Text(
                          'Ainda não há nenhuma\npublicação',
                          textAlign: TextAlign.center,
                          style: TextStyle(
                            color: text,
                            fontSize: 20,
                            fontWeight: FontWeight.w700,
                          ),
                        ),
                      ],
                    )
                  : Text(
                      'Nenhuma publicação marcada',
                      style: TextStyle(color: text.withOpacity(.6)),
                    ),
            ),
          ),
          InstagramBottomNav(
            darkMode: darkMode,
            selected: 4,
            onHome: onHome,
            onProfile: onProfile,
          ),
        ],
      ),
    );
  }
}

class InstagramStat extends StatelessWidget {
  final String value;
  final String label;

  const InstagramStat({super.key, required this.value, required this.label});

  @override
  Widget build(BuildContext context) => Column(
    children: [
      Text(
        value,
        style: const TextStyle(fontSize: 16, fontWeight: FontWeight.w700),
      ),
      Text(label, style: const TextStyle(fontSize: 12)),
    ],
  );
}

class InstagramBottomNav extends StatelessWidget {
  final bool darkMode;
  final int selected;
  final VoidCallback onHome;
  final VoidCallback onProfile;

  const InstagramBottomNav({
    super.key,
    required this.darkMode,
    required this.selected,
    required this.onHome,
    required this.onProfile,
  });

  @override
  Widget build(BuildContext context) {
    final text = darkMode ? Colors.white : const Color(0xFF111111);
    final icons = [
      Icons.home_outlined,
      Icons.search,
      Icons.add_box_outlined,
      Icons.favorite_border,
    ];
    return Container(
      height: 54,
      decoration: BoxDecoration(
        color: darkMode ? const Color(0xFF101114) : Colors.white,
        border: Border(top: BorderSide(color: text.withOpacity(.08))),
      ),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceAround,
        children: [
          IconButton(
            onPressed: onHome,
            icon: Icon(icons[0], color: text, size: 27),
          ),
          Icon(icons[1], color: text, size: 27),
          Icon(icons[2], color: text, size: 27),
          Icon(icons[3], color: text, size: 27),
          GestureDetector(
            onTap: onProfile,
            child: CircleAvatar(
              radius: 14,
              backgroundColor: selected == 4
                  ? text.withOpacity(.16)
                  : Colors.transparent,
              backgroundImage: const AssetImage(
                'assets/apps/instagram/avatar.png',
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class StoryBubble extends StatelessWidget {
  final String label;
  final bool isAdd;
  const StoryBubble({super.key, required this.label, this.isAdd = false});

  @override
  Widget build(BuildContext context) => SizedBox(
    width: 72,
    child: Column(
      children: [
        Stack(
          clipBehavior: Clip.none,
          children: [
            Container(
              padding: const EdgeInsets.all(3),
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                gradient: isAdd
                    ? null
                    : const LinearGradient(
                        colors: [
                          Color(0xFFFEDA75),
                          Color(0xFFD62976),
                          Color(0xFF4F5BD5),
                        ],
                      ),
              ),
              child: CircleAvatar(
                radius: 27,
                backgroundColor: Colors.grey.shade300,
                backgroundImage: isAdd
                    ? null
                    : const AssetImage('assets/apps/instagram/avatar.png'),
                child: isAdd
                    ? const Icon(Icons.person, color: Colors.white, size: 30)
                    : null,
              ),
            ),
            if (isAdd)
              Positioned(
                right: -1,
                bottom: -1,
                child: Container(
                  width: 23,
                  height: 23,
                  decoration: const BoxDecoration(
                    color: Color(0xFF2296E8),
                    shape: BoxShape.circle,
                  ),
                  child: const Icon(Icons.add, color: Colors.white, size: 17),
                ),
              ),
          ],
        ),
        const SizedBox(height: 5),
        Text(
          label,
          overflow: TextOverflow.ellipsis,
          style: const TextStyle(fontSize: 11),
        ),
      ],
    ),
  );
}

class BankScreen extends StatefulWidget {
  final bool darkMode;

  const BankScreen({super.key, required this.darkMode});

  @override
  State<BankScreen> createState() => _BankScreenState();
}

class _BankTransaction {
  final String desc;
  final int amount;
  final String date;

  const _BankTransaction(this.desc, this.amount, this.date);
}

class _BankScreenState extends State<BankScreen> {
  String _playerName = 'Jogador';
  int _cash = 0;
  int _balance = 0;
  bool _hasPin = false;
  bool _loading = true;
  String? _toastMsg;
  bool _toastOk = true;
  List<_BankTransaction> _history = [];

  // Overlay de PIN
  bool _showPinOverlay = false;
  bool _pinIsRegister = false;
  String _pinInput = '';

  // Toggle de visibilidade do saldo
  bool _balanceHidden = false;

  // Overlay de ação (sacar/depositar/transferir)
  bool _showActionOverlay = false;
  String _actionType = '';
  final _actionAmountCtrl = TextEditingController();
  final _actionTargetCtrl = TextEditingController();

  // Overlay de auth (senha pra confirmar transação)
  bool _showAuthOverlay = false;
  String _authPin = '';
  String _savedPin = ''; // PIN salvo na auth de cada ação

  @override
  void initState() {
    super.initState();
    _requestBankData();
    PhoneBridge.onBankData = _onBankData;
    PhoneBridge.onBankUpdate = _onBankUpdate;
  }

  @override
  void dispose() {
    PhoneBridge.onBankData = null;
    PhoneBridge.onBankUpdate = null;
    _actionAmountCtrl.dispose();
    _actionTargetCtrl.dispose();
    super.dispose();
  }

  void _requestBankData() {
    PhoneBridge.requestBankData();
  }

  void _onBankData(String json) {
    _applyBankPayload(json);
  }

  void _onBankUpdate(String json) {
    _applyBankPayload(json);
  }

  bool _authenticated = false; // se já digitou a senha nesta sessão
  bool _firstPayload = true; // primeira resposta do servidor (abertura)

  void _applyBankPayload(String json) {
    try {
      final data = jsonDecode(json) as Map<String, dynamic>;
      if (!mounted) return;
      setState(() {
        _playerName = data['player'] as String? ?? _playerName;
        _cash = data['cash'] as int? ?? _cash;
        _balance = data['balance'] as int? ?? _balance;
        _hasPin = data['hasPin'] as bool? ?? false;
        _loading = false;
        if (data['msg'] != null) {
          _toastMsg = data['msg'] as String;
          _toastOk = data['ok'] as bool? ?? true;
        }
        if (data['history'] is List) {
          _history = (data['history'] as List).map((e) {
            final m = e as Map<String, dynamic>;
            return _BankTransaction(
              m['desc'] as String? ?? '',
              m['amount'] as int? ?? 0,
              m['date'] as String? ?? '',
            );
          }).toList();
        }
        if (_firstPayload) {
          // Primeira resposta = abertura do banco
          _firstPayload = false;
          if (_hasPin) {
            // Já tem PIN -> mostra overlay de auth pra digitar a senha
            _showAuthOverlay = true;
            _authPin = '';
          } else {
            // Não tem PIN -> mostra overlay de registro
            _showPinOverlay = true;
            _pinIsRegister = true;
            _pinInput = '';
          }
        } else {
          // Respostas seguintes (resultado de verify_pin, transação, etc.)
          if (_hasPin && _toastOk) {
            // PIN verificado com sucesso -> libera acesso
            _authenticated = true;
            _showPinOverlay = false;
            _showAuthOverlay = false;
          } else if (!_toastOk) {
            // PIN errado ou erro -> mantém overlay de auth visível
            _authPin = '';
            _savedPin = '';
          }
        }
      });
      // Toast automático
      if (_toastMsg != null) {
        final msg = _toastMsg;
        final ok = _toastOk;
        Future.delayed(const Duration(seconds: 2), () {
          if (mounted && _toastMsg == msg) setState(() => _toastMsg = null);
        });
      }
    } catch (_) {
      if (mounted) setState(() => _loading = false);
    }
  }

  // ==================== Ações ====================
  // Já autenticou ao abrir o banco → abre o modal direto
  void _onSaque() {
    _pendingActionType = 'sacar';
    _openAction('sacar');
  }
  void _onDeposito() {
    _pendingActionType = 'depositar';
    _openAction('depositar');
  }
  void _onTransferir() {
    _pendingActionType = 'transferir';
    _openAction('transferir');
  }

  String _pendingActionType = '';

  void _showAuthForAction() {
    setState(() {
      _showAuthOverlay = true;
      _authPin = '';
    });
  }

  void _openAction(String type) {
    _actionAmountCtrl.clear();
    _actionTargetCtrl.clear();
    setState(() {
      _actionType = type;
      _showActionOverlay = true;
    });
  }

  void _confirmAction() {
    final amount = int.tryParse(_actionAmountCtrl.text) ?? 0;
    if (amount <= 0) return;
    _pendingAmount = amount;
    _pendingTarget = int.tryParse(_actionTargetCtrl.text) ?? 0;
    // Sempre pede PIN pra confirmar a transação
    setState(() {
      _showActionOverlay = false;
      _showAuthOverlay = true;
      _authPin = '';
    });
  }

  int _pendingAmount = 0;
  int _pendingTarget = 0;

  void _submitAuthPin() {
    if (_authPin.length != 4) return;
    final pin = int.tryParse(_authPin) ?? 0;

    // Se tem ação pendente, envia a transação pro servidor
    if (_pendingActionType.isNotEmpty && _pendingAmount > 0) {
      _sendTransaction(pin);
    } else if (_pendingActionType.isNotEmpty) {
      // Autenticou para uma ação mas ainda não digitou valor -> abre o modal
      _savedPin = _authPin; // salva o PIN pra usar depois
      setState(() {
        _showAuthOverlay = false;
        _authPin = '';
        _authenticated = true;
        _actionType = _pendingActionType;
        _showActionOverlay = true;
        _actionAmountCtrl.clear();
        _actionTargetCtrl.clear();
      });
    } else {
      // Autenticou só pra acessar (login)
      _savedPin = _authPin; // salva o PIN pra usar depois
      PhoneBridge.sendBankAction('verify_pin', {'pin': pin});
      setState(() {
        _showAuthOverlay = false;
        _authPin = '';
        _authenticated = true;
      });
    }
  }

  void _sendTransaction(int pin) {
    final actionMap = {
      'sacar': 'withdraw',
      'depositar': 'deposit',
      'transferir': 'transfer',
    };
    PhoneBridge.sendBankAction(actionMap[_pendingActionType] ?? '', {
      'amount': _pendingAmount,
      'pin': pin,
      'target': _pendingTarget,
    });
    setState(() {
      _showActionOverlay = false;
      _showAuthOverlay = false;
      _authPin = '';
      _pendingActionType = '';
      _pendingAmount = 0;
    });
  }

  String _formatMoney(int v) => '\$ ${v.toString().replaceAllMapped(RegExp(r'(\d{1,3})(?=(\d{3})+(?!\d))'), (m) => '${m[1]}.')}';

  @override
  Widget build(BuildContext context) {
    final text = widget.darkMode ? Colors.white : const Color(0xFF182230);
    final surface = widget.darkMode ? const Color(0xFF1C2430) : Colors.white;

    return Container(
      color: widget.darkMode ? const Color(0xFF101114) : const Color(0xFFF4F7FB),
      child: Stack(
        children: [
          // Conteúdo principal
          ListView(
            padding: const EdgeInsets.fromLTRB(18, 10, 18, 30),
            children: [
              // Header
              Row(
                children: [
                  const CircleAvatar(
                    radius: 21,
                    backgroundColor: Color(0xFFDCE7FF),
                    child: Icon(Icons.person, color: Color(0xFF155EEF)),
                  ),
                  const SizedBox(width: 10),
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(
                          'Olá, $_playerName',
                          style: TextStyle(
                            color: text,
                            fontSize: 16,
                            fontWeight: FontWeight.w700,
                          ),
                        ),
                        Text(
                          'Conta principal',
                          style: TextStyle(
                            color: text.withOpacity(.55),
                            fontSize: 12,
                          ),
                        ),
                      ],
                    ),
                  ),
                  Icon(Icons.notifications_none_rounded, color: text),
                ],
              ),
              const SizedBox(height: 22),
              // Card de saldo
              Container(
                padding: const EdgeInsets.all(20),
                decoration: BoxDecoration(
                  gradient: const LinearGradient(
                    colors: [Color(0xFF155EEF), Color(0xFF6A5AE0)],
                  ),
                  borderRadius: BorderRadius.circular(22),
                  boxShadow: [
                    BoxShadow(
                      color: const Color(0xFF155EEF).withOpacity(.24),
                      blurRadius: 18,
                      offset: const Offset(0, 8),
                    ),
                  ],
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text(
                      'Saldo disponível',
                      style: TextStyle(color: Colors.white70),
                    ),
                    const SizedBox(height: 6),
                    Text(
                      _balanceHidden ? '••••••' : _formatMoney(_balance),
                      style: const TextStyle(
                        color: Colors.white,
                        fontSize: 30,
                        fontWeight: FontWeight.w700,
                      ),
                    ),
                    const SizedBox(height: 14),
                    Row(
                      children: [
                        Text(
                          _balanceHidden ? 'Carteira: ••••' : 'Carteira: ${_formatMoney(_cash)}',
                          style: const TextStyle(color: Colors.white70, fontSize: 12),
                        ),
                        const Spacer(),
                        GestureDetector(
                          onTap: () => setState(() => _balanceHidden = !_balanceHidden),
                          child: Icon(
                            _balanceHidden ? Icons.visibility_off_outlined : Icons.visibility_outlined,
                            color: Colors.white70,
                            size: 19,
                          ),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
              const SizedBox(height: 20),
              // Botões de ação
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  GestureDetector(
                    onTap: _onSaque,
                    child: const BankAction(icon: Icons.arrow_downward_rounded, label: 'Sacar'),
                  ),
                  GestureDetector(
                    onTap: _onDeposito,
                    child: const BankAction(icon: Icons.arrow_upward_rounded, label: 'Depositar'),
                  ),
                  GestureDetector(
                    onTap: _onTransferir,
                    child: const BankAction(icon: Icons.swap_horiz_rounded, label: 'Transferir'),
                  ),
                  const BankAction(icon: Icons.more_horiz, label: 'Mais'),
                ],
              ),
              const SizedBox(height: 24),
              // Histórico
              Row(
                children: [
                  Expanded(
                    child: Text(
                      'Últimas movimentações',
                      style: TextStyle(
                        color: text,
                        fontSize: 18,
                        fontWeight: FontWeight.w700,
                      ),
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 7),
              if (_history.isEmpty)
                Padding(
                  padding: const EdgeInsets.symmetric(vertical: 20),
                  child: Center(
                    child: Text(
                      'Nenhuma movimentação recente',
                      style: TextStyle(color: text.withOpacity(.5), fontSize: 13),
                    ),
                  ),
                )
              else
                ..._history.map(
                  (item) => ListTile(
                    contentPadding: EdgeInsets.zero,
                    leading: const CircleAvatar(
                      backgroundColor: Color(0x1F00C853),
                      child: Icon(
                        Icons.arrow_downward_rounded,
                        color: Colors.green,
                        size: 20,
                      ),
                    ),
                    title: Text(
                      item.desc,
                      style: TextStyle(color: text, fontWeight: FontWeight.w600),
                    ),
                    subtitle: Text(
                      item.date,
                      style: TextStyle(color: text.withOpacity(.5), fontSize: 12),
                    ),
                    trailing: Text(
                      '${item.amount >= 0 ? '+' : '- '}\$ ${item.amount.abs()}',
                      style: const TextStyle(
                        color: Colors.green,
                        fontWeight: FontWeight.w700,
                        fontSize: 13,
                      ),
                    ),
                  ),
                ),
            ],
          ),
          // Toast
          if (_toastMsg != null)
            Positioned(
              top: 10,
              left: 18,
              right: 18,
              child: Material(
                color: Colors.transparent,
                child: Container(
                  padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
                  decoration: BoxDecoration(
                    color: _toastOk ? const Color(0xFF25D366) : const Color(0xFFE53935),
                    borderRadius: BorderRadius.circular(10),
                  ),
                  child: Text(
                    _toastMsg!,
                    style: const TextStyle(color: Colors.white, fontSize: 13),
                    textAlign: TextAlign.center,
                  ),
                ),
              ),
            ),
          // Overlay: registrar PIN
          if (_showPinOverlay)
            _buildPinOverlay(text, isRegister: true),
          // Overlay: ação (sacar/depositar/transferir)
          if (_showActionOverlay)
            _buildActionOverlay(text),
          // Overlay: auth (confirmar com PIN)
          if (_showAuthOverlay)
            _buildPinOverlay(text, isRegister: false),
        ],
      ),
    );
  }

  // ==================== Overlay de PIN ====================
  Widget _buildPinOverlay(Color text, {required bool isRegister}) {
    final pin = isRegister ? _pinInput : _authPin;
    return GestureDetector(
      onTap: () {}, // bloqueia clique
      child: Container(
        color: Colors.black54,
        child: Center(
          child: Container(
            width: 280,
            padding: const EdgeInsets.all(24),
            decoration: BoxDecoration(
              color: widget.darkMode ? const Color(0xFF1C2430) : Colors.white,
              borderRadius: BorderRadius.circular(20),
            ),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(
                  isRegister ? Icons.lock_outline : Icons.pin,
                  size: 36,
                  color: const Color(0xFF155EEF),
                ),
                const SizedBox(height: 12),
                Text(
                  isRegister ? 'Criar Senha de 4 dígitos' : 'Digite sua Senha',
                  style: TextStyle(color: text, fontWeight: FontWeight.w700, fontSize: 16),
                ),
                const SizedBox(height: 16),
                // Display do PIN
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: List.generate(4, (i) {
                    final filled = i < pin.length;
                    return Container(
                      width: 16,
                      height: 16,
                      margin: const EdgeInsets.symmetric(horizontal: 6),
                      decoration: BoxDecoration(
                        shape: BoxShape.circle,
                        color: filled ? const Color(0xFF155EEF) : text.withOpacity(.15),
                      ),
                    );
                  }),
                ),
                const SizedBox(height: 20),
                // Teclado numérico
                Wrap(
                  spacing: 8,
                  runSpacing: 8,
                  alignment: WrapAlignment.center,
                  children: List.generate(12, (i) {
                    if (i == 9) return const SizedBox(width: 52, height: 52); // espaço
                    if (i == 10) {
                      // 0
                      return _pinKey('0', pin, isRegister);
                    }
                    if (i == 11) {
                      // backspace
                      return GestureDetector(
                        onTap: () => setState(() {
                          if (isRegister) {
                            _pinInput = _pinInput.isNotEmpty ? _pinInput.substring(0, _pinInput.length - 1) : '';
                          } else {
                            _authPin = _authPin.isNotEmpty ? _authPin.substring(0, _authPin.length - 1) : '';
                          }
                        }),
                        child: Container(
                          width: 52,
                          height: 52,
                          decoration: BoxDecoration(
                            color: text.withOpacity(.08),
                            borderRadius: BorderRadius.circular(14),
                          ),
                          child: Icon(Icons.backspace_outlined, color: text, size: 20),
                        ),
                      );
                    }
                    final num = '${i + 1}';
                    return _pinKey(num, pin, isRegister);
                  }),
                ),
                const SizedBox(height: 16),
                SizedBox(
                  width: double.infinity,
                  child: FilledButton(
                    onPressed: pin.length == 4
                        ? () {
                            if (isRegister) {
                              // Registrar PIN
                              PhoneBridge.sendBankAction('register_pin', {
                                'pin': int.tryParse(_pinInput) ?? 0,
                              });
                              setState(() {
                                _pinInput = '';
                                _showPinOverlay = false;
                              });
                            } else {
                              // Confirmar transação
                              _submitAuthPin();
                            }
                          }
                        : null,
                    style: FilledButton.styleFrom(
                      backgroundColor: const Color(0xFF155EEF),
                    ),
                    child: Text(
                      isRegister ? 'Registrar Senha' : 'Confirmar',
                      style: const TextStyle(fontWeight: FontWeight.w700),
                    ),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }

  Widget _pinKey(String num, String currentPin, bool isRegister) {
    return GestureDetector(
      onTap: () => setState(() {
        if (currentPin.length < 4) {
          if (isRegister) {
            _pinInput += num;
          } else {
            _authPin += num;
          }
        }
      }),
      child: Container(
        width: 52,
        height: 52,
        decoration: BoxDecoration(
          color: widget.darkMode ? const Color(0xFF24262B) : Colors.grey.shade100,
          borderRadius: BorderRadius.circular(14),
        ),
        child: Center(
          child: Text(
            num,
            style: TextStyle(
              fontSize: 20,
              fontWeight: FontWeight.w600,
              color: widget.darkMode ? Colors.white : Colors.black87,
            ),
          ),
        ),
      ),
    );
  }

  // ==================== Overlay de Ação ====================
  Widget _buildActionOverlay(Color text) {
    final titles = {
      'sacar': 'Sacar Dinheiro',
      'depositar': 'Depositar Dinheiro',
      'transferir': 'Transferir Dinheiro',
    };
    return GestureDetector(
      onTap: () {},
      child: Container(
        color: Colors.black54,
        child: Center(
          child: Container(
            width: 300,
            padding: const EdgeInsets.all(24),
            decoration: BoxDecoration(
              color: widget.darkMode ? const Color(0xFF1C2430) : Colors.white,
              borderRadius: BorderRadius.circular(20),
            ),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                Text(
                  titles[_actionType] ?? '',
                  style: TextStyle(color: text, fontWeight: FontWeight.w700, fontSize: 18),
                ),
                const SizedBox(height: 16),
                if (_actionType == 'transferir')
                  TextField(
                    controller: _actionTargetCtrl,
                    keyboardType: TextInputType.number,
                    decoration: InputDecoration(
                      hintText: 'ID do jogador',
                      hintStyle: TextStyle(color: text.withOpacity(.4)),
                      border: OutlineInputBorder(borderRadius: BorderRadius.circular(10)),
                    ),
                  ),
                if (_actionType == 'transferir') const SizedBox(height: 12),
                TextField(
                  controller: _actionAmountCtrl,
                  keyboardType: TextInputType.number,
                  decoration: InputDecoration(
                    hintText: 'Valor',
                    hintStyle: TextStyle(color: text.withOpacity(.4)),
                    border: OutlineInputBorder(borderRadius: BorderRadius.circular(10)),
                  ),
                ),
                const SizedBox(height: 16),
                Row(
                  children: [
                    Expanded(
                      child: OutlinedButton(
                        onPressed: () => setState(() => _showActionOverlay = false),
                        child: const Text('Cancelar'),
                      ),
                    ),
                    const SizedBox(width: 12),
                    Expanded(
                      child: FilledButton(
                        onPressed: _confirmAction,
                        style: FilledButton.styleFrom(backgroundColor: const Color(0xFF155EEF)),
                        child: const Text('Continuar'),
                      ),
                    ),
                  ],
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class BankAction extends StatelessWidget {
  final IconData icon;
  final String label;
  const BankAction({super.key, required this.icon, required this.label});
  @override
  Widget build(BuildContext context) => Column(
    children: [
      Container(
        width: 54,
        height: 54,
        decoration: BoxDecoration(
          color: const Color(0xFFE8EEFF),
          borderRadius: BorderRadius.circular(17),
        ),
        child: Icon(icon, color: const Color(0xFF155EEF)),
      ),
      const SizedBox(height: 7),
      Text(
        label,
        style: const TextStyle(fontSize: 11, fontWeight: FontWeight.w600),
      ),
    ],
  );
}

class BankMiniCard extends StatelessWidget {
  final IconData icon;
  final String title;
  final String value;
  final Color surface;
  final Color text;
  const BankMiniCard({
    super.key,
    required this.icon,
    required this.title,
    required this.value,
    required this.surface,
    required this.text,
  });
  @override
  Widget build(BuildContext context) => Container(
    padding: const EdgeInsets.all(14),
    decoration: BoxDecoration(
      color: surface,
      borderRadius: BorderRadius.circular(18),
      border: Border.all(color: text.withOpacity(.08)),
    ),
    child: Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Icon(icon, color: const Color(0xFF155EEF)),
        const SizedBox(height: 12),
        Text(
          title,
          style: TextStyle(color: text.withOpacity(.65), fontSize: 12),
        ),
        const SizedBox(height: 3),
        Text(
          value,
          style: TextStyle(color: text, fontWeight: FontWeight.w700),
        ),
      ],
    ),
  );
}

class UberScreen extends StatefulWidget {
  final bool darkMode;

  const UberScreen({super.key, required this.darkMode});

  @override
  State<UberScreen> createState() => _UberScreenState();
}

class _UberScreenState extends State<UberScreen> {
  bool requested = false;
  int selectedRide = 0;

  @override
  Widget build(BuildContext context) {
    const rideAssets = ['uber_x', 'uber_confort', 'uber_motorcycle'];
    const rideNames = ['UberX', 'Confort', 'Moto'];
    final text = widget.darkMode ? Colors.white : Colors.black87;
    final surface = widget.darkMode ? const Color(0xFF202124) : Colors.white;

    return Stack(
      children: [
        Positioned.fill(
          child: Image.asset(
            'assets/apps/uber/uberx_top.png',
            fit: BoxFit.cover,
          ),
        ),
        Positioned.fill(
          child: Container(
            color: Colors.white.withOpacity(widget.darkMode ? .18 : .06),
          ),
        ),
        Positioned(
          top: 16,
          left: 16,
          right: 16,
          child: Container(
            padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 13),
            decoration: BoxDecoration(
              color: surface,
              borderRadius: BorderRadius.circular(14),
              boxShadow: [
                BoxShadow(color: Colors.black.withOpacity(.14), blurRadius: 16),
              ],
            ),
            child: const Row(
              children: [
                Icon(Icons.search),
                SizedBox(width: 12),
                Text(
                  'Para onde vamos?',
                  style: TextStyle(fontSize: 16, fontWeight: FontWeight.w500),
                ),
                Spacer(),
                Icon(Icons.access_time_rounded, size: 20),
              ],
            ),
          ),
        ),
        Positioned(
          left: 16,
          right: 16,
          bottom: 16,
          child: Container(
            padding: const EdgeInsets.fromLTRB(16, 12, 16, 16),
            decoration: BoxDecoration(
              color: surface,
              borderRadius: BorderRadius.circular(22),
              boxShadow: [
                BoxShadow(color: Colors.black.withOpacity(.18), blurRadius: 18),
              ],
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    const Icon(Icons.my_location_rounded, size: 18),
                    const SizedBox(width: 8),
                    Text(
                      'Sua localização atual',
                      style: TextStyle(
                        color: text,
                        fontWeight: FontWeight.w600,
                      ),
                    ),
                    const Spacer(),
                    Icon(Icons.close, size: 18, color: text),
                  ],
                ),
                const SizedBox(height: 15),
                Text(
                  'Escolha uma categoria',
                  style: TextStyle(
                    color: text,
                    fontSize: 16,
                    fontWeight: FontWeight.w700,
                  ),
                ),
                const SizedBox(height: 9),
                Row(
                  children: rideAssets.asMap().entries.map((entry) {
                    final index = entry.key;
                    final isSelected = selectedRide == index;
                    return Expanded(
                      child: GestureDetector(
                        onTap: () => setState(() => selectedRide = index),
                        child: Container(
                          margin: EdgeInsets.only(
                            right: index == rideAssets.length - 1 ? 0 : 7,
                          ),
                          padding: const EdgeInsets.symmetric(
                            vertical: 8,
                            horizontal: 7,
                          ),
                          decoration: BoxDecoration(
                            color: isSelected
                                ? Colors.black
                                : Colors.grey.withOpacity(.11),
                            borderRadius: BorderRadius.circular(12),
                          ),
                          child: Column(
                            children: [
                              Image.asset(
                                'assets/apps/uber/${entry.value}.png',
                                height: 34,
                              ),
                              const SizedBox(height: 3),
                              Text(
                                rideNames[index],
                                style: TextStyle(
                                  color: isSelected ? Colors.white : text,
                                  fontSize: 11,
                                  fontWeight: FontWeight.w600,
                                ),
                              ),
                            ],
                          ),
                        ),
                      ),
                    );
                  }).toList(),
                ),
                const SizedBox(height: 13),
                SizedBox(
                  width: double.infinity,
                  child: FilledButton(
                    onPressed: () => setState(() => requested = !requested),
                    style: FilledButton.styleFrom(
                      backgroundColor: Colors.black,
                      padding: const EdgeInsets.symmetric(vertical: 14),
                    ),
                    child: Text(
                      requested ? 'Motorista a caminho' : 'Confirmar Uber',
                      style: const TextStyle(
                        color: Colors.white,
                        fontWeight: FontWeight.w700,
                      ),
                    ),
                  ),
                ),
              ],
            ),
          ),
        ),
      ],
    );
  }
}

class CameraScreen extends StatefulWidget {
  const CameraScreen({super.key});

  @override
  State<CameraScreen> createState() => _CameraScreenState();
}

class _CameraScreenState extends State<CameraScreen> {
  bool flash = false;
  bool frontCamera = true;
  bool captured = false;
  bool busy = false;
  bool refreshingPreview = false;
  Uint8List? previewFrame;
  Uint8List? lastPhoto;
  Timer? previewTimer;
  String selectedMode = 'FOTO';

  @override
  void initState() {
    super.initState();
    unawaited(PhoneBridge.setGtaCameraMode(1));
    previewTimer = Timer.periodic(
      const Duration(milliseconds: 180),
      (_) => unawaited(_refreshPreview()),
    );
    unawaited(_refreshPreview());
  }

  @override
  void dispose() {
    previewTimer?.cancel();
    unawaited(PhoneBridge.setGtaCameraMode(0));
    super.dispose();
  }

  Future<void> _refreshPreview() async {
    if (refreshingPreview || !mounted) return;
    refreshingPreview = true;
    final frame = await PhoneBridge.captureGtaCamera();
    if (mounted && frame != null && frame.isNotEmpty) {
      setState(() => previewFrame = frame);
    }
    refreshingPreview = false;
  }

  Future<void> _switchCamera() async {
    if (busy) return;
    setState(() {
      busy = true;
      frontCamera = !frontCamera;
    });
    await PhoneBridge.setGtaCameraMode(frontCamera ? 1 : 2);
    if (mounted) setState(() => busy = false);
  }

  Future<void> _takePicture() async {
    if (busy || selectedMode == 'VÍDEO') return;
    setState(() {
      busy = true;
      captured = true;
    });
    final photo = await PhoneBridge.captureGtaCamera();
    await Future<void>.delayed(const Duration(milliseconds: 90));
    if (mounted) {
      setState(() {
        lastPhoto = photo ?? lastPhoto;
        captured = false;
        busy = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Container(
      color: Colors.transparent,
      child: Stack(
        children: [
          Positioned.fill(
            child: previewFrame == null
                ? const ColoredBox(color: Colors.black)
                : Image.memory(
                    previewFrame!,
                    fit: BoxFit.cover,
                    gaplessPlayback: true,
                  ),
          ),
          Positioned(
            top: 0,
            left: 0,
            right: 0,
            height: 58,
            child: ColoredBox(color: Colors.black.withOpacity(.82)),
          ),
          Positioned(
            top: 10,
            left: 14,
            right: 14,
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                CameraCircle(
                  icon: flash ? Icons.flash_on : Icons.flash_off,
                  onTap: () => setState(() => flash = !flash),
                ),
                CameraCircle(icon: Icons.settings_outlined, onTap: () {}),
                CameraCircle(
                  icon: frontCamera
                      ? Icons.flip_camera_ios_outlined
                      : Icons.flip_camera_ios,
                  onTap: _switchCamera,
                ),
              ],
            ),
          ),
          Positioned(
            left: 0,
            right: 0,
            bottom: 0,
            height: 132,
            child: ColoredBox(color: Colors.black.withOpacity(.86)),
          ),
          Positioned(
            left: 0,
            right: 0,
            bottom: 88,
            child: Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: ['RETRATO', 'FOTO', 'VÍDEO'].map((mode) {
                final selected = selectedMode == mode;
                return GestureDetector(
                  onTap: () => setState(() => selectedMode = mode),
                  child: Container(
                    margin: const EdgeInsets.symmetric(horizontal: 10),
                    padding: const EdgeInsets.symmetric(
                      horizontal: 13,
                      vertical: 6,
                    ),
                    decoration: BoxDecoration(
                      border: selected
                          ? Border.all(color: Colors.white, width: 1.5)
                          : null,
                      borderRadius: BorderRadius.circular(18),
                    ),
                    child: Text(
                      mode,
                      style: TextStyle(
                        color: Colors.white.withOpacity(selected ? 1 : .68),
                        fontSize: 11,
                        fontWeight: selected
                            ? FontWeight.w700
                            : FontWeight.w500,
                      ),
                    ),
                  ),
                );
              }).toList(),
            ),
          ),
          Positioned(
            left: 20,
            right: 20,
            bottom: 17,
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Container(
                  width: 45,
                  height: 45,
                  decoration: BoxDecoration(
                    color: Colors.white.withOpacity(.18),
                    borderRadius: BorderRadius.circular(10),
                    border: Border.all(color: Colors.white24),
                    image: lastPhoto == null
                        ? null
                        : DecorationImage(
                            image: MemoryImage(lastPhoto!),
                            fit: BoxFit.cover,
                          ),
                  ),
                  child: lastPhoto == null
                      ? const Icon(
                          Icons.photo_library_outlined,
                          color: Colors.white,
                          size: 22,
                        )
                      : null,
                ),
                GestureDetector(
                  onTap: _takePicture,
                  child: Container(
                    width: 70,
                    height: 70,
                    decoration: BoxDecoration(
                      shape: BoxShape.circle,
                      border: Border.all(color: Colors.white, width: 5),
                    ),
                    child: Container(
                      margin: const EdgeInsets.all(5),
                      decoration: const BoxDecoration(
                        shape: BoxShape.circle,
                        color: Colors.white,
                      ),
                    ),
                  ),
                ),
                CameraCircle(icon: Icons.bolt_outlined, onTap: () {}),
              ],
            ),
          ),
          if (captured)
            Positioned.fill(
              child: IgnorePointer(
                child: ColoredBox(color: Colors.white.withOpacity(.8)),
              ),
            ),
        ],
      ),
    );
  }
}

class CameraCircle extends StatelessWidget {
  final IconData icon;
  final VoidCallback onTap;

  const CameraCircle({super.key, required this.icon, required this.onTap});

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: Container(
        width: 42,
        height: 42,
        decoration: BoxDecoration(
          color: Colors.black.withOpacity(.38),
          shape: BoxShape.circle,
        ),
        child: Icon(icon, color: Colors.white, size: 22),
      ),
    );
  }
}

class PhoneScreen extends StatefulWidget {
  final bool darkMode;

  const PhoneScreen({super.key, required this.darkMode});

  @override
  State<PhoneScreen> createState() => _PhoneScreenState();
}

class _PhoneScreenState extends State<PhoneScreen> {
  String number = '';
  bool calling = false;
  final keys = const [
    ('1', ''),
    ('2', 'ABC'),
    ('3', 'DEF'),
    ('4', 'GHI'),
    ('5', 'JKL'),
    ('6', 'MNO'),
    ('7', 'PQRS'),
    ('8', 'TUV'),
    ('9', 'WXYZ'),
    ('*', ''),
    ('0', '+'),
    ('#', ''),
  ];

  void _addDigit(String digit) {
    setState(() {
      calling = false;
      number += digit;
    });
  }

  void _deleteDigit() {
    if (number.isEmpty) return;
    setState(() => number = number.substring(0, number.length - 1));
  }

  @override
  Widget build(BuildContext context) {
    final text = widget.darkMode ? Colors.white : const Color(0xFF15181D);
    final keySurface = widget.darkMode ? const Color(0xFF212329) : Colors.white;
    return Container(
      color: widget.darkMode
          ? const Color(0xFF101114)
          : const Color(0xFFF9FAFB),
      child: Column(
        children: [
          const SizedBox(height: 18),
          Text(
            calling
                ? 'Ligando...'
                : (number.isEmpty ? 'Digite um número' : number),
            style: TextStyle(
              color: text,
              fontSize: 25,
              fontWeight: FontWeight.w400,
            ),
          ),
          const Spacer(),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 38),
            child: GridView.builder(
              shrinkWrap: true,
              physics: const NeverScrollableScrollPhysics(),
              itemCount: keys.length,
              gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                crossAxisCount: 3,
                mainAxisSpacing: 10,
                crossAxisSpacing: 20,
                mainAxisExtent: 64,
              ),
              itemBuilder: (_, index) {
                final item = keys[index];
                return GestureDetector(
                  onTap: () => _addDigit(item.$1),
                  child: Container(
                    width: 58,
                    height: 58,
                    decoration: BoxDecoration(
                      color: keySurface,
                      shape: BoxShape.circle,
                      boxShadow: [
                        BoxShadow(
                          color: Colors.black.withOpacity(.08),
                          blurRadius: 6,
                          offset: const Offset(0, 2),
                        ),
                      ],
                    ),
                    child: Center(
                      child: Text(
                        item.$1,
                        style: TextStyle(color: text, fontSize: 25),
                      ),
                    ),
                  ),
                );
              },
            ),
          ),
          const SizedBox(height: 15),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              GestureDetector(
                onTap: () => setState(() => calling = !calling),
                child: Container(
                  width: 62,
                  height: 62,
                  decoration: const BoxDecoration(
                    color: Color(0xFF20B45B),
                    shape: BoxShape.circle,
                  ),
                  child: Icon(
                    calling ? Icons.call_end : Icons.call,
                    color: Colors.white,
                    size: 28,
                  ),
                ),
              ),
              const SizedBox(width: 34),
              IconButton(
                onPressed: _deleteDigit,
                icon: Icon(
                  Icons.backspace_outlined,
                  color: text.withOpacity(.7),
                ),
              ),
            ],
          ),
          const SizedBox(height: 20),
        ],
      ),
    );
  }
}

class OlxScreen extends StatelessWidget {
  final bool darkMode;

  const OlxScreen({super.key, required this.darkMode});

  @override
  Widget build(BuildContext context) {
    final text = darkMode ? Colors.white : const Color(0xFF252525);
    return Container(
      color: darkMode ? const Color(0xFF171717) : const Color(0xFFF7F7F7),
      child: Column(
        children: [
          Container(
            color: const Color(0xFF6A35FF),
            padding: const EdgeInsets.fromLTRB(16, 12, 16, 18),
            child: Column(
              children: [
                const Row(
                  children: [
                    Text(
                      'olx',
                      style: TextStyle(
                        color: Colors.white,
                        fontSize: 28,
                        fontWeight: FontWeight.w900,
                        letterSpacing: -2,
                      ),
                    ),
                    Spacer(),
                    Icon(Icons.notifications_none, color: Colors.white),
                    SizedBox(width: 17),
                    Icon(Icons.chat_bubble_outline, color: Colors.white),
                  ],
                ),
                const SizedBox(height: 13),
                Container(
                  padding: const EdgeInsets.symmetric(
                    horizontal: 13,
                    vertical: 12,
                  ),
                  decoration: BoxDecoration(
                    color: Colors.white,
                    borderRadius: BorderRadius.circular(10),
                  ),
                  child: const Row(
                    children: [
                      Icon(Icons.search, color: Colors.black54),
                      SizedBox(width: 8),
                      Text(
                        'O que você está procurando?',
                        style: TextStyle(color: Colors.black45),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
          Padding(
            padding: const EdgeInsets.fromLTRB(15, 17, 15, 9),
            child: Row(
              children: [
                Text(
                  'Ofertas perto de você',
                  style: TextStyle(
                    color: text,
                    fontSize: 18,
                    fontWeight: FontWeight.w700,
                  ),
                ),
                const Spacer(),
                const Text(
                  'Ver mais',
                  style: TextStyle(
                    color: Color(0xFF6A35FF),
                    fontWeight: FontWeight.w600,
                    fontSize: 12,
                  ),
                ),
              ],
            ),
          ),
          Expanded(
            child: GridView.count(
              crossAxisCount: 2,
              padding: const EdgeInsets.symmetric(horizontal: 12),
              crossAxisSpacing: 10,
              mainAxisSpacing: 12,
              childAspectRatio: .78,
              children: [
                ProductCard(
                  image: 'assets/apps/olx/bike.png',
                  title: 'Bicicleta urbana',
                  price: 'R\$ 780',
                  text: text,
                ),
                ProductCard(
                  image: 'assets/apps/olx/home.png',
                  title: 'Apartamento 2 quartos',
                  price: 'R\$ 1.650/mês',
                  text: text,
                ),
                ProductCard(
                  image: 'assets/apps/olx/photo.png',
                  title: 'Câmera profissional',
                  price: 'R\$ 2.300',
                  text: text,
                ),
                ProductCard(
                  image: 'assets/apps/olx/bike.png',
                  title: 'Mountain bike',
                  price: 'R\$ 1.200',
                  text: text,
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

class ProductCard extends StatelessWidget {
  final String image;
  final String title;
  final String price;
  final Color text;
  const ProductCard({
    super.key,
    required this.image,
    required this.title,
    required this.price,
    required this.text,
  });
  @override
  Widget build(BuildContext context) => Container(
    decoration: BoxDecoration(
      color: Theme.of(context).cardColor,
      borderRadius: BorderRadius.circular(13),
      border: Border.all(color: text.withOpacity(.08)),
    ),
    clipBehavior: Clip.antiAlias,
    child: Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Expanded(
          child: Stack(
            fit: StackFit.expand,
            children: [
              Image.asset(image, fit: BoxFit.cover),
              Positioned(
                top: 8,
                right: 8,
                child: CircleAvatar(
                  radius: 15,
                  backgroundColor: Colors.white.withOpacity(.88),
                  child: const Icon(
                    Icons.favorite_border,
                    size: 18,
                    color: Colors.black87,
                  ),
                ),
              ),
            ],
          ),
        ),
        Padding(
          padding: const EdgeInsets.fromLTRB(10, 8, 8, 10),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                title,
                maxLines: 1,
                overflow: TextOverflow.ellipsis,
                style: TextStyle(
                  color: text,
                  fontWeight: FontWeight.w600,
                  fontSize: 13,
                ),
              ),
              const SizedBox(height: 3),
              Text(
                price,
                style: TextStyle(
                  color: text,
                  fontWeight: FontWeight.w800,
                  fontSize: 15,
                ),
              ),
            ],
          ),
        ),
      ],
    ),
  );
}

class SettingsScreen extends StatelessWidget {
  final bool darkMode;
  final String wallpaper;
  final VoidCallback onToggleTheme;
  final VoidCallback onChangeWallpaper;
  const SettingsScreen({
    super.key,
    required this.darkMode,
    required this.wallpaper,
    required this.onToggleTheme,
    required this.onChangeWallpaper,
  });

  @override
  Widget build(BuildContext context) {
    final text = darkMode ? Colors.white : const Color(0xFF1B1D22);
    final secondary = darkMode ? Colors.white60 : Colors.black54;
    return Container(
      color: darkMode ? const Color(0xFF111214) : const Color(0xFFF5F6F8),
      child: ListView(
        padding: const EdgeInsets.fromLTRB(18, 8, 18, 24),
        children: [
          Text(
            'Ajustes rápidos',
            style: TextStyle(
              color: text,
              fontSize: 24,
              fontWeight: FontWeight.w800,
            ),
          ),
          const SizedBox(height: 17),
          Container(
            padding: const EdgeInsets.all(16),
            decoration: BoxDecoration(
              color: darkMode ? const Color(0xFF24262B) : Colors.white,
              borderRadius: BorderRadius.circular(20),
            ),
            child: Row(
              children: [
                CircleAvatar(
                  radius: 27,
                  backgroundColor: const Color(0xFFDEE5FF),
                  child: Icon(
                    Icons.person,
                    color: Colors.indigo.shade600,
                    size: 31,
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'Alex Oliveira',
                        style: TextStyle(
                          color: text,
                          fontSize: 17,
                          fontWeight: FontWeight.w700,
                        ),
                      ),
                      Text(
                        'Conta local do dispositivo',
                        style: TextStyle(color: secondary, fontSize: 12),
                      ),
                    ],
                  ),
                ),
                const Icon(Icons.chevron_right),
              ],
            ),
          ),
          const SizedBox(height: 22),
          Text(
            'Personalização',
            style: TextStyle(color: secondary, fontWeight: FontWeight.w700),
          ),
          const SizedBox(height: 8),
          SettingsTile(
            icon: Icons.dark_mode_outlined,
            title: 'Modo escuro',
            subtitle: darkMode ? 'Ativado' : 'Desativado',
            text: text,
            trailing: Switch(
              value: darkMode,
              onChanged: (_) => onToggleTheme(),
            ),
          ),
          SettingsTile(
            icon: Icons.wallpaper_outlined,
            title: 'Papel de parede',
            subtitle: 'Toque para trocar',
            text: text,
            trailing: ClipRRect(
              borderRadius: BorderRadius.circular(8),
              child: Image.asset(
                wallpaper,
                width: 42,
                height: 42,
                fit: BoxFit.cover,
              ),
            ),
            onTap: onChangeWallpaper,
          ),
          const SizedBox(height: 20),
          Text(
            'Sistema',
            style: TextStyle(color: secondary, fontWeight: FontWeight.w700),
          ),
          const SizedBox(height: 8),
          SettingsTile(
            icon: Icons.wifi_rounded,
            title: 'Wi-Fi',
            subtitle: 'Casa_5G • Conectado',
            text: text,
            trailing: const Icon(Icons.chevron_right),
          ),
          SettingsTile(
            icon: Icons.bluetooth_outlined,
            title: 'Bluetooth',
            subtitle: 'Ativado',
            text: text,
            trailing: const Icon(Icons.chevron_right),
          ),
          SettingsTile(
            icon: Icons.info_outline_rounded,
            title: 'Sobre o aparelho',
            subtitle: 'Phone Simulator 1.0',
            text: text,
            trailing: const Icon(Icons.chevron_right),
          ),
        ],
      ),
    );
  }
}

class SettingsTile extends StatelessWidget {
  final IconData icon;
  final String title;
  final String subtitle;
  final Color text;
  final Widget trailing;
  final VoidCallback? onTap;
  const SettingsTile({
    super.key,
    required this.icon,
    required this.title,
    required this.subtitle,
    required this.text,
    required this.trailing,
    this.onTap,
  });
  @override
  Widget build(BuildContext context) => ListTile(
    onTap: onTap,
    contentPadding: const EdgeInsets.symmetric(vertical: 3),
    leading: Container(
      width: 42,
      height: 42,
      decoration: BoxDecoration(
        color: text.withOpacity(.08),
        borderRadius: BorderRadius.circular(13),
      ),
      child: Icon(icon, color: text),
    ),
    title: Text(
      title,
      style: TextStyle(color: text, fontWeight: FontWeight.w600),
    ),
    subtitle: Text(
      subtitle,
      style: TextStyle(color: text.withOpacity(.52), fontSize: 12),
    ),
    trailing: trailing,
  );
}

class MessagesScreen extends StatelessWidget {
  final bool darkMode;

  const MessagesScreen({super.key, required this.darkMode});

  @override
  Widget build(BuildContext context) {
    final text = darkMode ? Colors.white : Colors.black87;
    final conversations = [
      ('Mariana Santos', 'M', Colors.pink, 'Você: combinado!', '10:42'),
      ('João Pedro', 'J', Colors.blue, 'Você recebeu uma foto', 'Ontem'),
      ('Clínica Odonto', 'C', Colors.teal, 'Lembrete de consulta', 'Seg'),
    ];
    return Container(
      color: darkMode ? const Color(0xFF101114) : const Color(0xFFF5F7FA),
      child: ListView(
        padding: const EdgeInsets.all(18),
        children: [
          Text(
            'Mensagens',
            style: TextStyle(
              fontSize: 27,
              fontWeight: FontWeight.w800,
              color: text,
            ),
          ),
          const SizedBox(height: 17),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 11),
            decoration: BoxDecoration(
              color: darkMode ? const Color(0xFF24262B) : Colors.white,
              borderRadius: BorderRadius.circular(15),
            ),
            child: Row(
              children: [
                Icon(Icons.search, color: text.withOpacity(.55)),
                const SizedBox(width: 9),
                Text(
                  'Buscar conversas',
                  style: TextStyle(color: text.withOpacity(.45)),
                ),
              ],
            ),
          ),
          const SizedBox(height: 16),
          ...conversations.map(
            (conversation) => ListTile(
              contentPadding: EdgeInsets.zero,
              leading: CircleAvatar(
                radius: 24,
                backgroundColor: conversation.$3,
                child: Text(
                  conversation.$2,
                  style: const TextStyle(
                    color: Colors.white,
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ),
              title: Text(
                conversation.$1,
                style: TextStyle(color: text, fontWeight: FontWeight.w700),
              ),
              subtitle: Text(
                conversation.$4,
                style: TextStyle(color: text.withOpacity(.55)),
              ),
              trailing: Text(
                conversation.$5,
                style: TextStyle(color: text.withOpacity(.45), fontSize: 12),
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class GalleryScreen extends StatelessWidget {
  final bool darkMode;
  const GalleryScreen({super.key, required this.darkMode});
  @override
  Widget build(BuildContext context) {
    final images = [
      'assets/apps/instagram/teste_post.png',
      'assets/apps/olx/bike.png',
      'assets/apps/olx/home.png',
      'assets/apps/uber/uberx_top.png',
      'assets/wallpapers/homeBg.png',
      'assets/apps/camera/1_1.png',
    ];
    return Container(
      color: darkMode ? Colors.black : Colors.white,
      child: GridView.builder(
        padding: const EdgeInsets.all(4),
        itemCount: images.length,
        gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
          crossAxisCount: 3,
          crossAxisSpacing: 3,
          mainAxisSpacing: 3,
        ),
        itemBuilder: (_, index) =>
            Image.asset(images[index], fit: BoxFit.cover),
      ),
    );
  }
}

class CalculatorScreen extends StatefulWidget {
  final bool darkMode;
  const CalculatorScreen({super.key, required this.darkMode});
  @override
  State<CalculatorScreen> createState() => _CalculatorScreenState();
}

class _CalculatorScreenState extends State<CalculatorScreen> {
  String _display = '0';
  String _operator = '';
  double _first = 0;
  bool _waitingSecond = false;

  void _tap(String value) {
    setState(() {
      if (value == 'C') {
        _display = '0';
        _operator = '';
        _first = 0;
        _waitingSecond = false;
      } else if (value == '⌫') {
        if (_display.length > 1) {
          _display = _display.substring(0, _display.length - 1);
        } else {
          _display = '0';
        }
      } else if (value == '%') {
        final current = double.tryParse(_display) ?? 0;
        _display = (current / 100).toString();
        _cleanDisplay();
      } else if (value == '=') {
        // Avaliar expressao inteira da esquerda pra direita
        final result = _evalExpression(_display);
        _display = _cleanNumber(result);
        _operator = '';
        _first = 0;
        _waitingSecond = false;
      } else if (value == '+' || value == '-' || value == '×' || value == '÷') {
        // Se ja tem operador no final, trocar
        if (_display.isNotEmpty && ['+', '-', '×', '÷'].contains(_display[_display.length - 1])) {
          _display = _display.substring(0, _display.length - 1) + value;
          _operator = value;
          // Nao atualiza _first, mantem o que ja tem
        } else {
          _first = double.tryParse(_display) ?? 0;
          _display = '$_display$value';
          _operator = value;
        }
        _waitingSecond = true;
      } else {
        // Número ou ponto
        if (_waitingSecond) {
          _display = value == '.' ? '${_display}0.' : '$_display$value';
          _waitingSecond = false;
        } else {
          if (value == '.' && _display.contains('.')) return;
          _display = _display == '0' && value != '.' ? value : '$_display$value';
        }
      }
    });
  }

  String _cleanNumber(double v) {
    if (v == v.roundToDouble() && v.abs() < 1e15) return v.toInt().toString();
    return v.toStringAsFixed(8).replaceFirst(RegExp(r'0+$'), '').replaceFirst(RegExp(r'\.$'), '');
  }

  void _cleanDisplay() {
    final v = double.tryParse(_display) ?? 0;
    _display = _cleanNumber(v);
  }

  // Avalia expressao da esquerda pra direita: 1+1+2-1 = 3
  double _evalExpression(String expr) {
    // Separar numeros e operadores
    final numbers = <double>[];
    final ops = <String>[];
    var current = '';
    for (var i = 0; i < expr.length; i++) {
      final c = expr[i];
      if (c == '+' || c == '-' || c == '×' || c == '÷') {
        numbers.add(double.tryParse(current) ?? 0);
        ops.add(c);
        current = '';
      } else {
        current += c;
      }
    }
    numbers.add(double.tryParse(current) ?? 0);
    if (numbers.isEmpty) return 0;
    var result = numbers[0];
    for (var i = 0; i < ops.length; i++) {
      switch (ops[i]) {
        case '+': result += numbers[i + 1]; break;
        case '-': result -= numbers[i + 1]; break;
        case '×': result *= numbers[i + 1]; break;
        case '÷': result /= numbers[i + 1] != 0 ? numbers[i + 1] : 1; break;
      }
    }
    return result;
  }

  @override
  Widget build(BuildContext context) {
    final text = widget.darkMode ? Colors.white : Colors.black87;
    const buttons = [
      'C',
      '÷',
      '×',
      '⌫',
      '7',
      '8',
      '9',
      '-',
      '4',
      '5',
      '6',
      '+',
      '1',
      '2',
      '3',
      '=',
      '0',
      '.',
      '%',
      '',
    ];
    return Container(
      color: widget.darkMode
          ? const Color(0xFF111214)
          : const Color(0xFFF7F7F7),
      child: Column(
        children: [
          Expanded(
            child: Align(
              alignment: Alignment.bottomRight,
              child: Padding(
                padding: const EdgeInsets.all(25),
                child: Text(
                  _display,
                  style: TextStyle(
                    color: text,
                    fontSize: 45,
                    fontWeight: FontWeight.w300,
                  ),
                ),
              ),
            ),
          ),
          GridView.builder(
            shrinkWrap: true,
            padding: const EdgeInsets.all(15),
            itemCount: buttons.length,
            gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
              crossAxisCount: 4,
              mainAxisSpacing: 10,
              crossAxisSpacing: 10,
              childAspectRatio: 1.2,
            ),
            itemBuilder: (_, index) {
              final label = buttons[index];
              if (label.isEmpty) return const SizedBox.shrink();
              final isAction = [
                'C',
                '÷',
                '×',
                '⌫',
                '-',
                '+',
                '=',
                '%',
              ].contains(label);
              return FilledButton(
                onPressed: () => _tap(label),
                style: FilledButton.styleFrom(
                  backgroundColor: label == '='
                      ? const Color(0xFF5E5CE6)
                      : (isAction
                            ? (widget.darkMode
                                  ? const Color(0xFF353741)
                                  : const Color(0xFFE5E7EB))
                            : (widget.darkMode
                                  ? const Color(0xFF24262B)
                                  : Colors.white)),
                  foregroundColor: label == '=' ? Colors.white : text,
                  shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(18),
                  ),
                ),
                child: Text(
                  label,
                  style: const TextStyle(
                    fontSize: 20,
                    fontWeight: FontWeight.w600,
                  ),
                ),
              );
            },
          ),
        ],
      ),
    );
  }
}

class NotesScreen extends StatefulWidget {
  final bool darkMode;
  const NotesScreen({super.key, required this.darkMode});
  @override
  State<NotesScreen> createState() => _NotesScreenState();
}

class _NotesScreenState extends State<NotesScreen> {
  final controller = TextEditingController(
    text:
        'Ideias para o fim de semana\n\n- Caminhar no parque\n- Comprar ingredientes para o jantar\n- Separar fotos da viagem',
  );
  @override
  void dispose() {
    controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => Container(
    color: widget.darkMode ? const Color(0xFF111214) : const Color(0xFFFFFCF0),
    padding: const EdgeInsets.fromLTRB(20, 10, 20, 20),
    child: Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(
          'Notas',
          style: TextStyle(
            color: widget.darkMode ? Colors.white : Colors.black87,
            fontSize: 27,
            fontWeight: FontWeight.w800,
          ),
        ),
        const SizedBox(height: 13),
        Expanded(
          child: TextField(
            controller: controller,
            expands: true,
            maxLines: null,
            minLines: null,
            textAlignVertical: TextAlignVertical.top,
            style: TextStyle(
              color: widget.darkMode ? Colors.white : Colors.black87,
              fontSize: 17,
              height: 1.55,
            ),
            decoration: const InputDecoration(
              border: InputBorder.none,
              hintText: 'Escreva uma nota...',
            ),
          ),
        ),
        Align(
          alignment: Alignment.centerRight,
          child: FilledButton.icon(
            onPressed: () => ScaffoldMessenger.of(context).showSnackBar(
              const SnackBar(content: Text('Nota salva neste dispositivo.')),
            ),
            icon: const Icon(Icons.check),
            label: const Text('Salvar'),
          ),
        ),
      ],
    ),
  );
}

class GenericAppScreen extends StatelessWidget {
  final AppDefinition app;
  final bool darkMode;

  const GenericAppScreen({
    super.key,
    required this.app,
    required this.darkMode,
  });

  @override
  Widget build(BuildContext context) {
    final text = darkMode ? Colors.white : Colors.black87;
    return Container(
      color: darkMode ? const Color(0xFF111214) : const Color(0xFFF6F7F9),
      child: Center(
        child: Padding(
          padding: const EdgeInsets.all(30),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              ClipRRect(
                borderRadius: BorderRadius.circular(28),
                child: Image.asset(app.icon, width: 92, height: 92),
              ),
              const SizedBox(height: 20),
              Text(
                app.name,
                style: TextStyle(
                  color: text,
                  fontSize: 26,
                  fontWeight: FontWeight.w800,
                ),
              ),
              const SizedBox(height: 8),
              Text(
                'Este aplicativo está pronto para receber novas telas e integrações.',
                textAlign: TextAlign.center,
                style: TextStyle(
                  color: text.withOpacity(.58),
                  fontSize: 14,
                  height: 1.4,
                ),
              ),
              const SizedBox(height: 22),
              FilledButton.icon(
                onPressed: () {},
                icon: const Icon(Icons.download_done_rounded),
                label: const Text('Abrir demonstração'),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
