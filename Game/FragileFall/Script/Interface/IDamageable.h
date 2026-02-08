#pragma once

namespace game
{
	class IDamageable
    {
    public:
        virtual ~IDamageable() = default;

        virtual void TakeDamage(float damage, bool isShieldPierce = false) = 0;
    };
}