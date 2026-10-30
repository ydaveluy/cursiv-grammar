// Declarations: import, typedef, enum, class, mixin, interface, funcdef,
// virtprop and var at script scope, plus a namespace wrapping more of the
// same.

// Function attributes are optional and may form a sequence.
import void notify(int level) explicit from "log.as";

namespace Game
{
    typedef int32 EntityId;

    enum Empty;

    enum Status
    {
        Idle,
        Running = 1,
        Stopped = Running + 1
    }

    shared abstract class Base
    {
        int hp;
        void tick() { }
    }

    class Forward;

    class Entity : Base, IUpdatable
    {
        private int hp;
        protected float speed = 1.0;

        int score, lives = 3;

        Entity()
        {
            hp = 100;
        }

        ~Entity()
        {
        }

        void damage(int amount, int kind = 0)
        {
            hp -= amount;
        }

        int& getScoreRef() const
        {
            return score;
        }

        // A parameter direction modifier: a plain `&`, then each of
        // `&in`/`&out`/`&inout`.
        void copyInto(int &output, int &in src, int &out dst, int &inout acc)
        {
            output = src;
        }

        // FUNCATTR on an ordinary method, and an `auto`-typed parameter.
        void tag(auto value) override
        {
        }

        int health
        {
            get const property { return hp; }
            set property { hp = value; }
        }
    }

    mixin class Flying
    {
        float altitude;
    }

    external shared interface IUpdatable
    {
        void update(float dt) const;
    }

    interface IEmpty;

    external funcdef void Callback(int code);

    Callback@ onDone;
}

int globalCounter = 0;
array<int> values(4);

// The `?` any-type placeholder.
? anyValue;

// A function itself may carry `shared`/`external` modifiers, not only a
// class/interface/enum/funcdef declaration.
shared void globalTick()
{
}
