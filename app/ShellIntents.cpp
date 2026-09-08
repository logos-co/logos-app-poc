#include "ShellIntents.h"

#include "IntentRegistry.h"

namespace ShellIntents {

void registerWith(IntentRegistry* registry,
                  const QString& shellModuleName,
                  const QString& displayName,
                  const QString& iconSource)
{
    if (!registry) return;

    registry->registerShellUses(shellModuleName,
                                { QStringLiteral("packages.show"),
                                  QStringLiteral("packages.install") });

    QStringList provides = kNavigationIntents;
    provides += kPackageConfirmIntents;

    registry->registerShellProvider(shellModuleName, provides,
                                    kNavigationIntents, displayName, iconSource);

    for (const QString& destructive : kRestrictedToPackageManagerUi)
        registry->restrictIntentToRequesters(
            destructive, { QStringLiteral("package_manager_ui") });
}

} // namespace ShellIntents
