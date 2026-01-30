#pragma once

#include "Framework/Asset/AssetData.h"

namespace engine
{
	class GameObject;

    struct Socket
    {
        std::string name;
        std::string parentBoneName;
        DirectX::SimpleMath::Vector3 localPosition;
        DirectX::SimpleMath::Quaternion localRotation;
        DirectX::SimpleMath::Vector3 localScale;
        DirectX::SimpleMath::Matrix localMatrix;

        void UpdateLocalMatrix();
        void DecomposeLocalMatrix();
    };

    struct SocketInstance
    {
        Socket info;
        Matrix worldMatrix;
    };

    struct AttachedSocketObject
    {
        Ptr<GameObject> obj;
        std::string socketName;
    };

    class SocketData : public AssetData
    {
    private:
        std::vector<Socket> m_sockets;
        std::unordered_map<std::string, Socket*> m_socketMap;

    public:
        SocketData() = default;
        virtual ~SocketData() override = default;

        void Create(const std::string& filePath);

        const std::vector<Socket>& GetSockets() const { return m_sockets; }
        const Socket* GetSocket(const std::string& name) const;
        void SetSockets(const std::vector<Socket>& sockets);

        void Save(const std::string& filePath);
    };
}
