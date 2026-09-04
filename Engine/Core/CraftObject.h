#pragma once

#include <Core/Core.h>
#include <memory>

// 커스텀 타입 시스템을 제공하는 최상위 클래스
namespace Craft
{
class CRAFT_API CraftObject
{
  public:
    virtual ~CraftObject() = default;

    virtual size_t GetType() const = 0;

    virtual bool Is(size_t id) const
    {
        return false;
    }
    template <typename T> bool IsTypeOf() const
    {
        return Is(T::TypeId());
    }
};

// 형변환 함수
template <typename T, typename U> std::shared_ptr<T> Cast(const std::shared_ptr<U>& object)
{
    // 예외처리
    if (!object)
    {
        return nullptr;
    }
    if (object->Is(T::TypeId()))
    {
        return std::static_pointer_cast<T>(object);
    }
    return nullptr;
}
} // namespace Craft

#define TYPE_DECLARATIONS(Type, ParentType)                                                                            \
    using super = ParentType;                                                                                          \
                                                                                                                       \
  protected:                                                                                                           \
    static size_t TypeIdClass()                                                                                        \
    {                                                                                                                  \
        static int runTimeTypeId = 0;                                                                                  \
        return reinterpret_cast<size_t>(&runTimeTypeId);                                                               \
    }                                                                                                                  \
                                                                                                                       \
  public:                                                                                                              \
    static size_t TypeId()                                                                                             \
    {                                                                                                                  \
        return Type::TypeIdClass();                                                                                    \
    }                                                                                                                  \
    virtual size_t GetType() const override                                                                            \
    {                                                                                                                  \
        return Type::TypeIdClass();                                                                                    \
    }                                                                                                                  \
    virtual bool Is(size_t id) const override                                                                          \
    {                                                                                                                  \
        return (id == TypeIdClass()) ? true : ParentType::Is(id);                                                      \
    }
