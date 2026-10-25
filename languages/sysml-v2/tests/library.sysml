// Covers: package, library package (plain and standard), alias, dependency,
// the standalone relationship statements (specialization/subtype,
// subclassification, subsetting, redefinition, typing, conjugation,
// disjoining, featureInverting, typeFeaturing), the non-feature type
// keywords (type, classifier, class, struct, datatype, assoc, assoc
// struct, behavior, interaction, metaclass), soft keywords used as names,
// every inline specialization form (:>, :>>, ~, references, subsets,
// crosses, redefines, typed by, defined by), multiplicity ranges, <short>
// name, import ::**.
package Library {
    private import Base::**;

    alias <Q> QtyKind for Base::Quantity;

    dependency FromLib from Library::Sensor to Library::Actuator;

    type Thing;

    classifier Sensor :> Thing {
        attribute reading : Real;
    }

    class Widget {
        attribute serial : String;
    }

    struct Point {
        attribute x : Real;
        attribute y : Real;
    }

    datatype Coordinate;

    assoc Links {
        end a : Sensor;
        end b : Widget;
    }

    assoc struct WeightedLink {
        attribute weight : Real;
    }

    behavior GenericBehavior;

    interaction Handshake;

    metaclass Tag;

    part def Actuator {
        // A comma-separated list continues one specialization kind
        // without repeating its operator.
        part multi : Sensor, Widget;

        attribute type : String;
        attribute multiplicity : Integer;
        attribute var : Boolean;
        attribute language : String;
        attribute locale : String;
        attribute crosses : Integer;
        attribute step : Real;
        attribute feature : String;
        attribute behavior : String;
        attribute function : String;
        attribute member : String;
        attribute predicate : String;
        attribute interaction : String;
        attribute metaclass : String;

        part origin : Point[1];
        part sample : Point[0..*];

        ref part upstream references Sensor;
        part shadow subsets upstream;
        part crossPart crosses upstream;

        part <T> tagged : Widget;
    }

    part def RedefinedActuator :> Actuator {
        part renamed redefines origin;
    }

    part def PortLike ~ Actuator;

    attribute def Ratio :> ScalarValue {
        attribute num : Real typed by Base::Quantity;
        attribute den : Real defined by Base::Quantity;
    }

    specialization SpecialA subtype Widget :> Sensor;
    specialization SubclassB subclassifier Widget :> Sensor;
    specialization SubsetC subset Widget :> Sensor;
    specialization RedefD redefinition Widget :>> Sensor;
    specialization TypingE typing Widget : Sensor;
    conjugation ConjF conjugate Widget ~ Sensor;
    disjoining DisjG disjoint Widget from Sensor;
    featuring Widget by Sensor;
    featuring named of Widget by Sensor;
    inverting InvH inverse Widget of Sensor;

    standard library package StdLib {
        part def Base;
    }

    library package ExtraLib {
        part def Extra;
    }
}
