// Own test document: imports, namespaces, enums, aliases and classes --
// UCLASS/UPROPERTY/UFUNCTION annotations, base classes, access specifiers,
// bit-fields, operator overloads, mixins and property accessors.

import Engine.Math;
import Engine.Math.Vector;
import float Distance(float a, float b) from "MathModule";

namespace MyGame
{
	enum EColor : int32
	{
		Red,
		Green,
		Blue = 5,
	};

	enum EForwardOnly;

	MyBaseAlias = float;

	UCLASS(Abstract)
	class ABaseThing
	{
		int32 Secret;

		UPROPERTY(EditAnywhere, Category = "Stats")
		float Speed = 1.0f;

		UFUNCTION(BlueprintCallable)
		void TakeDamage(int32 Amount)
		{
			Secret -= Amount;
		}

		int32 GetHealth() const
		{
			return Secret;
		}

		mixin void MixedIn() { }

		void Overridable() override final property { }

		~ABaseThing() { }

		ABaseThing operator=(const ABaseThing &Other)
		{
			Secret = Other.Secret;
			return this;
		}

		int32 operator+(int32 Other)
		{
			return Secret + Other;
		}

		int32 operator[](int32 Index)
		{
			return Secret + Index;
		}

		void operator()()
		{
			Secret = 0;
		}
	};

	UCLASS()
	class ADerivedThing : public ABaseThing
	{
		private:
		int32 Hidden;

		public:
		uint8 Flags : 4;
		uint8 : 4;

		default Hidden = 50;
	};
}

namespace MyGameAlias = MyGame;
