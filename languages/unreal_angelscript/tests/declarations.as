// Own test document: imports, namespaces, enums and classes --
// UCLASS/UPROPERTY/UFUNCTION annotations, base classes, access specifiers,
// opAdd/opAssign methods, mixins and function attributes.

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

		ABaseThing opAssign(const ABaseThing &Other)
		{
			Secret = Other.Secret;
			return this;
		}

		int32 opAdd(int32 Other)
		{
			return Secret + Other;
		}

		int32 opIndex(int32 Index)
		{
			return Secret + Index;
		}

		void opCall()
		{
			Secret = 0;
		}
	};

	UCLASS()
	class ADerivedThing : ABaseThing
	{
		private int32 Hidden;

		public uint8 Flags = 0;

		default Hidden = 50;
	};
}
