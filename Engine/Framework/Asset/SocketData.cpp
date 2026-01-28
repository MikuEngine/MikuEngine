#include "EnginePCH.h"
#include "Framework/Asset/SocketData.h"

#include <fstream>

#include "Core/System/VirtualFileSystem.h"

namespace engine
{
    void Socket::UpdateLocalMatrix()
    {
        localMatrix = Matrix::CreateScale(localScale) *
                      Matrix::CreateFromQuaternion(localRotation) *
                      Matrix::CreateTranslation(localPosition);
    }

    void Socket::DecomposeLocalMatrix()
    {
        Vector3 scale;
        Vector3 pos;
        Quaternion rot;

        localMatrix.Decompose(scale, rot, pos);

        localScale = scale;
        localRotation = rot;
        localPosition = pos;
    }

    void SocketData::Create(const std::string& filePath)
    {
        auto& vfs = VirtualFileSystem::Get();
        std::vector<uint8_t> fileData;

        if (!vfs.LoadFile(filePath, fileData))
        {
            LOG_INFO("{} 파일 열기 실패 - SocketData", filePath);
            return;
        }

        json j = json::parse(fileData.begin(), fileData.end());

        if (j.is_array())
        {
            m_sockets.clear();
            m_socketMap.clear();

            m_sockets.reserve(j.size());

            for (const auto& item : j)
            {
                Socket socket;
                JsonGet(item, "name", socket.name);
                JsonGet(item, "parent_bone", socket.parentBoneName, std::string(""));
                JsonGet(item, "position", socket.localPosition, Vector3::Zero);
                JsonGet(item, "rotation", socket.localRotation, Quaternion::Identity);
                JsonGet(item, "scale", socket.localScale, Vector3::One);

                socket.UpdateLocalMatrix();
                m_sockets.push_back(socket);
            }

            for (auto& socket : m_sockets)
            {
                m_socketMap[socket.name] = &socket;
            }
        }
    }

    const Socket* SocketData::GetSocket(const std::string& name) const
    {
        auto it = m_socketMap.find(name);
        return (it != m_socketMap.end()) ? it->second : nullptr;
    }

    void SocketData::SetSockets(const std::vector<Socket>& sockets)
    {
        m_sockets = sockets;
        m_socketMap.clear();
        for (auto& s : m_sockets) m_socketMap[s.name] = &s;
    }

    void SocketData::Save(const std::string& filePath)
    {
        json j = json::array();

        for (const auto& socket : m_sockets)
        {
            json item;
            item["name"] = socket.name;
            item["parent_bone"] = socket.parentBoneName;

            item["position"] = { {"x", socket.localPosition.x}, {"y", socket.localPosition.y}, {"z", socket.localPosition.z} };
            item["rotation"] = { {"x", socket.localRotation.x}, {"y", socket.localRotation.y}, {"z", socket.localRotation.z}, {"w", socket.localRotation.w} };
            item["scale"] = { {"x", socket.localScale.x}, {"y", socket.localScale.y}, {"z", socket.localScale.z} };

            j.push_back(item);
        }

        std::ofstream file(filePath);
        if (file.is_open())
        {
            file << j.dump(4);
            file.close();
        }
        else
        {
            LOG_INFO("Failed to create file: {} (Check folder exists)", filePath);
        }
    }
}
