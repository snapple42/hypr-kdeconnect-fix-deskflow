#include "portal_backend.hpp"
#include "wayland_input.hpp"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDebug>
#include <QThread>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("hypr-kdeconnect-portal"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    const QStringList args = QCoreApplication::arguments();
    if (args.size() == 4 && args.at(1) == QStringLiteral("--self-test-motion")) {
        bool okX = false;
        bool okY = false;
        const double dx = args.at(2).toDouble(&okX);
        const double dy = args.at(3).toDouble(&okY);
        if (!okX || !okY) {
            qCritical() << "usage: hypr-kdeconnect-portal --self-test-motion <dx> <dy>";
            return 2;
        }
        hkcf::WaylandInput input;
        if (!input.pointerMotion(dx, dy)) {
            qCritical() << input.lastError();
            return 1;
        }
        QThread::msleep(150);
        return 0;
    }

    if (args.size() == 4 && args.at(1) == QStringLiteral("--self-test-absolute")) {
        bool okX = false;
        bool okY = false;
        const double x = args.at(2).toDouble(&okX);
        const double y = args.at(3).toDouble(&okY);
        if (!okX || !okY) {
            qCritical() << "usage: hypr-kdeconnect-portal --self-test-absolute <x> <y>";
            return 2;
        }
        hkcf::WaylandInput input;
        if (!input.pointerMotionAbsolute(x, y)) {
            qCritical() << input.lastError();
            return 1;
        }
        QThread::msleep(150);
        return 0;
    }

    if (args.size() == 4 && args.at(1) == QStringLiteral("--self-test-scroll")) {
        bool okX = false;
        bool okY = false;
        const double dx = args.at(2).toDouble(&okX);
        const double dy = args.at(3).toDouble(&okY);
        if (!okX || !okY) {
            qCritical() << "usage: hypr-kdeconnect-portal --self-test-scroll <dx> <dy>";
            return 2;
        }
        hkcf::WaylandInput input;
        if (!input.pointerAxis(dx, dy)) {
            qCritical() << input.lastError();
            return 1;
        }
        QThread::msleep(150);
        return 0;
    }

    if (args.size() == 4 && args.at(1) == QStringLiteral("--self-test-scroll-discrete")) {
        bool okAxis = false;
        bool okSteps = false;
        const uint axis = args.at(2).toUInt(&okAxis);
        const int steps = args.at(3).toInt(&okSteps);
        if (!okAxis || !okSteps) {
            qCritical() << "usage: hypr-kdeconnect-portal --self-test-scroll-discrete <axis:0|1> <steps>";
            return 2;
        }
        hkcf::WaylandInput input;
        if (!input.pointerAxisDiscrete(axis, steps)) {
            qCritical() << input.lastError();
            return 1;
        }
        QThread::msleep(150);
        return 0;
    }

    if (args.size() >= 3 && args.at(1) == QStringLiteral("--self-test-key")) {
        QList<std::uint32_t> keycodes;
        for (int i = 2; i < args.size(); ++i) {
            bool ok = false;
            const uint keycode = args.at(i).toUInt(&ok);
            if (!ok) {
                qCritical() << "usage: hypr-kdeconnect-portal --self-test-key <evdev keycode> [<evdev keycode>...]";
                return 2;
            }
            keycodes.append(keycode);
        }

        hkcf::WaylandInput input;
        for (const std::uint32_t keycode : keycodes) {
            if (!input.keyboardKeycode(keycode, true)) {
                qCritical() << input.lastError();
                return 1;
            }
            QThread::msleep(20);
        }
        QThread::msleep(50);
        for (auto it = keycodes.crbegin(); it != keycodes.crend(); ++it) {
            if (!input.keyboardKeycode(*it, false)) {
                qCritical() << input.lastError();
                return 1;
            }
            QThread::msleep(20);
        }
        QThread::msleep(150);
        return 0;
    }

    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qCritical() << "failed to connect to session bus";
        return 1;
    }

    constexpr auto serviceName = "org.freedesktop.impl.portal.desktop.hypr_kdeconnect";
    if (!bus.registerService(QString::fromLatin1(serviceName))) {
        qCritical() << "failed to own D-Bus service" << serviceName << bus.lastError().message();
        return 1;
    }

    hkcf::PortalBackend backend;
    if (!bus.registerVirtualObject(QStringLiteral("/org/freedesktop/portal/desktop"), &backend, QDBusConnection::SubPath)) {
        qCritical() << "failed to export portal object" << bus.lastError().message();
        return 1;
    }

    qInfo() << "hypr-kdeconnect RemoteDesktop portal backend running";
    return app.exec();
}
