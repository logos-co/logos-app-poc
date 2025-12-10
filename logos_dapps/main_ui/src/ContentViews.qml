import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    Rectangle {
        anchors.fill: parent
        color: "#1e1e1e"
    }

    // Content views stack (indices 1-3 from backend, mapped to 0-2 here)
    // Index 0 (Apps/MDI) is handled by the C++ MdiView widget
    StackLayout {
        id: contentStack
        anchors.fill: parent
        
        // Map backend index: 1=Dashboard, 2=Modules, 3=Settings
        // to internal index: 0=Dashboard, 1=Modules, 2=Settings
        currentIndex: Math.max(0, backend.currentViewIndex - 1)

        // Dashboard (backend index 1 -> internal index 0)
        DashboardView {
            id: dashboardView
        }

        // Modules (backend index 2 -> internal index 1)
        ModulesView {
            id: modulesView
        }

        // Settings (backend index 3 -> internal index 2)
        SettingsView {
            id: settingsView
        }
    }
}

