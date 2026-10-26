import 'package:flutter_test/flutter_test.dart';

import 'package:phone_flutter/main.dart';

void main() {
  testWidgets('renders the phone home screen', (tester) async {
    await tester.pumpWidget(const PhoneSimulator());
    await tester.pumpAndSettle();

    expect(find.textContaining('24°'), findsOneWidget);
    expect(find.text('WhatsApp'), findsOneWidget);
    expect(find.text('São Paulo  •  24°'), findsOneWidget);
  });
}
