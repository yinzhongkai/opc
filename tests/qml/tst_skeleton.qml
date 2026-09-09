import QtQuick
import QtTest

TestCase {
    name: "QmlSkeleton"

    Item {
        id: fixture
        width: 64
        height: 36
    }

    function test_qml_engine_constructs_item() {
        compare(fixture.width, 64)
        compare(fixture.height, 36)
    }
}
