#pragma once

#include <functional>
#include <map>
#include <string>
#include <memory>
#include <set>

#include "Common/Utility/Singleton.h"

namespace engine
{
    class Component;

    class ComponentFactory :
        public Singleton<ComponentFactory>
    {
    private:
        using Creator = std::function<std::unique_ptr<Component>()>;
        
        std::map<std::string, Creator> m_registry;
        std::set<std::string> m_scriptNames;

    private:
        ComponentFactory() = default;

    public:
        void Register(const std::string& name, Creator creator);
        void RegisterScript(const std::string& name, Creator creator);
        bool IsScript(const std::string& name) const;

        std::unique_ptr<Component> Create(const std::string& name);
        const std::map<std::string, Creator>& GetRegistry() const;


    private:
        friend class Singleton<ComponentFactory>;
    };


#define DEFINE_COMPONENT_TYPE(type, baseType)                                       \
    public:                                                                        \
        static int GetStaticTypeID()                                                \
        {                                                                           \
            static int s_typeID = engine::detail::GetNextComponentTypeID();         \
                                                                                    \
            return s_typeID;                                                        \
        }                                                                           \
                                                                                    \
        virtual int GetTypeID() const override                                      \
        {                                                                           \
            return GetStaticTypeID();                                               \
        }                                                                           \
                                                                                    \
        virtual const char* GetTypeName() const override                            \
        {                                                                           \
            return #type;                                                           \
        }                                                                           \
                                                                                    \
        virtual bool IsA(int typeID) const override                                 \
        {                                                                           \
            if (typeID == GetStaticTypeID())                                        \
            {                                                                       \
                return true;                                                        \
            }                                                                       \
                                                                                    \
            return baseType::IsA(typeID);                                           \
        }                                                                           \
                                                                                    \
    private:


#define REGISTER_COMPONENT(type, baseType)                              \
        DEFINE_COMPONENT_TYPE(type, baseType)                           \
    private:                                                            \
        struct Registrar                                                \
        {                                                               \
            Registrar()                                                 \
            {                                                           \
                type::GetStaticTypeID();                                \
                engine::ComponentFactory::Get().Register(#type, []()    \
                    {                                                   \
                        return std::make_unique<type>();                \
                    }                                                   \
                );                                                      \
            }                                                           \
        };                                                              \
        inline static Registrar s_registrar;

#define REGISTER_SCRIPT(type, baseType)                               \
        DEFINE_COMPONENT_TYPE(type, baseType)                         \
    private:                                                          \
        struct Registrar                                              \
        {                                                             \
            Registrar()                                               \
            {                                                         \
                type::GetStaticTypeID();                              \
                engine::ComponentFactory::Get().RegisterScript(#type, \
                    []() { return std::make_unique<type>(); }         \
                );                                                    \
            }                                                         \
        };                                                            \
        inline static Registrar s_registrar;

}