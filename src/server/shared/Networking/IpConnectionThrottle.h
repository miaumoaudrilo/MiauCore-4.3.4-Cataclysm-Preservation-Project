/*
 * MiauCore - Límite de conexiones nuevas por IP (anti-DoS a nivel de aplicación).
 *
 * Cuenta las conexiones entrantes por dirección IP dentro de una ventana de tiempo.
 * Si una IP abre más conexiones que el límite configurado en esa ventana, las conexiones
 * sobrantes se rechazan de inmediato (antes de tocar la base de datos). Esto frena las
 * inundaciones de conexiones contra el authserver y el worldserver.
 *
 * Es por proceso: authserver y worldserver son procesos distintos, cada uno con su propia
 * instancia y su propia configuración (leída de su .conf). Todo el acceso está protegido
 * por un mutex, así que es seguro desde varios hilos de red.
 */

#ifndef TRINITY_IP_CONNECTION_THROTTLE_H
#define TRINITY_IP_CONNECTION_THROTTLE_H

#include "Define.h"
#include "Config.h"
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

class IpConnectionThrottle
{
public:
    static IpConnectionThrottle& Instance()
    {
        static IpConnectionThrottle instance;
        return instance;
    }

    // Registra un intento de conexión desde 'ip'. Devuelve true si se permite,
    // false si esa IP superó el límite en la ventana actual.
    bool AllowConnection(std::string const& ip)
    {
        std::lock_guard<std::mutex> guard(_mutex);

        if (!_configured)
            LoadConfig();

        if (_max <= 0) // 0 o negativo = control desactivado
            return true;

        auto now = std::chrono::steady_clock::now();
        auto window = std::chrono::milliseconds(_windowMs);

        // Limpieza oportunista para que el mapa no crezca sin fin.
        if (_entries.size() > 8192)
            Cleanup(now, window);

        Entry& e = _entries[ip];
        if (now - e.windowStart >= window)
        {
            e.windowStart = now;
            e.count = 0;
        }
        ++e.count;
        return e.count <= uint32(_max);
    }

private:
    IpConnectionThrottle() = default;

    struct Entry
    {
        std::chrono::steady_clock::time_point windowStart;
        uint32 count = 0;
    };

    void LoadConfig()
    {
        // Mismas claves en worldserver.conf y authserver.conf (cada proceso lee el suyo).
        _max = sConfigMgr->GetIntDefault("Connection.Throttle.Max", 30);
        _windowMs = uint32(sConfigMgr->GetIntDefault("Connection.Throttle.WindowMs", 1000));
        if (_windowMs == 0)
            _windowMs = 1000;
        _configured = true;
    }

    void Cleanup(std::chrono::steady_clock::time_point now, std::chrono::milliseconds window)
    {
        for (auto it = _entries.begin(); it != _entries.end(); )
        {
            if (now - it->second.windowStart >= window * 4)
                it = _entries.erase(it);
            else
                ++it;
        }
    }

    std::mutex _mutex;
    std::unordered_map<std::string, Entry> _entries;
    int32 _max = 30;
    uint32 _windowMs = 1000;
    bool _configured = false;
};

#define sIpConnectionThrottle IpConnectionThrottle::Instance()

#endif // TRINITY_IP_CONNECTION_THROTTLE_H
