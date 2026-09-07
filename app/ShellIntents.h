#pragma once

#include <QString>
#include <QStringList>

class IntentRegistry;

namespace ShellIntents {

inline const QStringList kPackageConfirmIntents = {
    QStringLiteral("basecamp.packages.confirm_install"),
    QStringLiteral("basecamp.packages.confirm_uninstall"),
    QStringLiteral("basecamp.packages.confirm_upgrade"),
};

// Of those, the ones no third party has a legitimate reason to raise.
inline const QStringList kRestrictedToPackageManagerUi = {
    QStringLiteral("basecamp.packages.confirm_uninstall"),
    QStringLiteral("basecamp.packages.confirm_upgrade"),
};

// PURE NAVIGATION, and hand-offs to a one. `ok` means "you are there", not "we
// are done", so the broker must not bounce the user back out of a destination
// it was asked to take them to.
inline const QStringList kNavigationIntents = {
    QStringLiteral("basecamp.repositories.manage"),
    QStringLiteral("basecamp.settings.open"),
    QStringLiteral("basecamp.apps.open"),
    QStringLiteral("basecamp.apps.launch"),
};

// Bring a named app forward: `{ "app": "wallet_ui" }`.
inline const QString kAppLaunchIntent = QStringLiteral("basecamp.apps.launch");
inline const QString kAppLaunchParam  = QStringLiteral("app");

// Declare the shell's provides, hand-offs, uses and requester restrictions.
void registerWith(IntentRegistry* registry,
                  const QString& shellModuleName,
                  const QString& displayName,
                  const QString& iconSource);

} // namespace ShellIntents
