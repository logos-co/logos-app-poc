import QtQuick
import QtTest

// Directory import, not `import Basecamp.AppManager` — see tst_IntentInstallDialog.
import "../../src/Basecamp/AppManager"

// The version picker in the Required Packages list sat in a fixed 110px
// column, so a repository whose versions carry a git-describe suffix
// ("2.0.0-32.gf8ab37c1" — logos-modules-dev publishes one per commit) showed
// an ellipsis in the closed control AND in the dropdown, which is only as wide
// as the control. The column now sizes to the widest version it has to
// display: shared across rows so they stay aligned, clamped so it cannot
// squeeze the description out.
TestCase {
    id: testCase
    name: "VersionColumn"
    when: windowShown
    visible: true
    width: 700
    height: 400

    readonly property string longVersion: "2.0.0-32.gf8ab37c1"

    Component {
        id: rowComp
        PackageRowDelegate { width: 600; height: 56 }
    }

    Component {
        id: dialogComp
        AddApplicationDialog {}
    }

    // dynamicRoles so a role can hold the catalog's version list verbatim.
    ListModel {
        id: packagesModel
        dynamicRoles: true
        Component.onCompleted: {
            append({ name: "chat_ui", displayName: "Chat UI", description: "",
                     versions: testCase.catalogVersions(["0.2.2"]),
                     toVersion: "0.2.2", action: "install" })
            append({ name: "delivery_module", displayName: "Delivery",
                     description: "",
                     versions: testCase.catalogVersions([testCase.longVersion]),
                     toVersion: testCase.longVersion, action: "install" })
        }
    }

    function catalogVersions(list) {
        var out = []
        for (var i = 0; i < list.length; ++i)
            out.push({ manifest: { version: list[i] } })
        return out
    }

    function makeRow(versions, columnWidth) {
        var props = {
            appRow: {
                name: "delivery_module",
                displayName: "Delivery",
                description: "Logos Delivery core module",
                versions: catalogVersions(versions),
                toVersion: versions[0],
                action: "install",
            },
        }
        if (columnWidth !== undefined) props.versionColumnWidth = columnWidth
        var row = rowComp.createObject(testCase, props)
        verify(row, "row delegate created")
        waitForRendering(row)
        return row
    }

    // The picker's own label — not one of the hidden width probes, which also
    // carry a "v." text.
    function findLabel(row) {
        var combo = findCombo(row)
        return combo ? combo.contentLabel : null
    }

    function findCombo(row) {
        var found = null
        function walk(item) {
            if (found || !item) return
            if (item.popup !== undefined && item.displayText !== undefined) {
                found = item
                return
            }
            for (var i = 0; i < item.children.length; ++i) walk(item.children[i])
        }
        walk(row)
        return found
    }

    function test_column_need_grows_with_the_longest_version() {
        var shortRow = makeRow(["0.2.1"])
        var longRow  = makeRow(["0.2.1", testCase.longVersion])
        verify(shortRow.versionContentWidth > 0, "a row with versions reports a need")
        verify(longRow.versionContentWidth > shortRow.versionContentWidth,
               "the longer version needs more room (" + longRow.versionContentWidth
               + " vs " + shortRow.versionContentWidth + ")")
        // The picker offers every version, so the need covers the longest one
        // rather than whichever happens to be selected.
        verify(longRow.versionContentWidth > 110,
               "a git-describe version outgrows the old fixed column")
        shortRow.destroy()
        longRow.destroy()
    }

    // The regression: at the old fixed 110 the label was cut. At the width the
    // row asks for, it must not be.
    function test_reported_need_shows_the_version_uncut() {
        var probe = makeRow([testCase.longVersion])
        var row = makeRow([testCase.longVersion], probe.versionContentWidth)
        var label = findLabel(row)
        verify(label, "the picker's label was found")
        compare(label.text, "v." + testCase.longVersion, "the full version is the text")
        compare(label.truncated, false, "no ellipsis at the width the row asked for")
        probe.destroy()
        row.destroy()
    }

    function test_old_fixed_width_would_have_truncated() {
        var row = makeRow([testCase.longVersion], 110)
        var label = findLabel(row)
        verify(label, "the picker's label was found")
        verify(label.truncated, "110px still truncates — so the need is real")
        row.destroy()
    }

    // The dropdown is as wide as the closed control, so a column that fits the
    // label fits the entries too — they carry no "v." prefix.
    function test_dropdown_entries_are_not_cut_either() {
        var probe = makeRow([testCase.longVersion])
        var row = makeRow(["0.2.1", testCase.longVersion], probe.versionContentWidth)
        var combo = findCombo(row)
        verify(combo, "the picker was found")
        combo.popup.open()
        waitForRendering(row)
        tryVerify(function() { return combo.popup.opened }, 2000, "the dropdown opened")
        var list = combo.popupListView
        verify(list, "the dropdown's list was found")
        var seen = 0
        for (var i = 0; i < list.contentItem.children.length; ++i) {
            var item = list.contentItem.children[i]
            var text = item && item.contentItem
            if (!text || text.truncated === undefined) continue
            seen += 1
            compare(text.truncated, false,
                    "dropdown entry '" + text.text + "' is not cut")
        }
        verify(seen > 0, "the dropdown rendered its entries")
        combo.popup.close()
        probe.destroy()
        row.destroy()
    }

    // The rows report, the dialog folds: one width for the whole list, so a
    // long version in any row cannot leave the columns ragged.
    function test_dialog_folds_the_widest_row_and_shares_it() {
        var dlg = dialogComp.createObject(testCase)
        verify(dlg, "dialog created")
        dlg.requiredPackagesModel = packagesModel
        dlg.openWith({ name: "delivery_module", displayName: "Delivery" })
        tryVerify(function() { return dlg.versionColumnWidth > 110 }, 3000,
                  "the dialog grew its version column past the old fixed one")

        var rows = []
        function walk(item) {
            if (!item) return
            if (item.versionContentWidth !== undefined) rows.push(item)
            for (var i = 0; i < item.children.length; ++i) walk(item.children[i])
        }
        walk(dlg.contentItem)
        compare(rows.length, 2, "both rows rendered")

        var widest = 0
        for (var i = 0; i < rows.length; ++i) {
            widest = Math.max(widest, rows[i].versionContentWidth)
            compare(rows[i].versionColumnWidth, dlg.versionColumnWidth,
                    "every row is given the folded width")
        }
        compare(dlg.versionColumnWidth, Math.ceil(widest),
                "which is exactly what the widest row asked for")

        dlg.close()
        dlg.destroy()
    }

    function test_clamp_floors_and_caps() {
        var dlg = dialogComp.createObject(testCase)
        verify(dlg, "dialog created")
        compare(dlg.clampVersionColumn(0), 110, "rows without versions keep the old width")
        compare(dlg.clampVersionColumn(60), 110, "never narrower than 110")
        compare(dlg.clampVersionColumn(150.2), 151, "rounded up to whole pixels")
        compare(dlg.clampVersionColumn(4000), 200, "capped so the description survives")
        compare(dlg.versionColumnWidth, 110, "starts at the floor")
        dlg.destroy()
    }

    // The cap has to clear what the dev catalog actually publishes, or the fix
    // stops at an ellipsis for exactly the repository that motivated it.
    function test_cap_clears_a_dev_catalog_version() {
        var row = makeRow([testCase.longVersion])
        var dlg = dialogComp.createObject(testCase)
        compare(dlg.clampVersionColumn(row.versionContentWidth),
                Math.ceil(row.versionContentWidth),
                "a logos-modules-dev version fits under the cap ("
                + row.versionContentWidth + ")")
        row.destroy()
        dlg.destroy()
    }
}
