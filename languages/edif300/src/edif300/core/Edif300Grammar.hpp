// The edif300 language: its grammar. EDIF 3 0 0 is one uniform syntax --
// nested parenthesized forms, `(keyword arg...)` -- so every one of the
// grammar's ~670 named constructs lowers to one AST shape (ast::Form) built
// by the same two macros; only the entry point and the three lexical tokens
// are written by hand.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "edif300/core/ast.hpp"

namespace edif300 {

struct Edif300 : cursiv::GrammarSpec {
  static constexpr auto Special =
      cr<R"(!#$&*+,./:;<=>?@[]^`{|}~\\-)">;
  static constexpr auto Letter = cr<"A-Za-z">;

  struct IDENTIFIER
      : terminal<many(Special) + many(d) + (Letter | lit<"&"> | lit<"_">) +
                 many(Letter | d | lit<"_"> | Special)> {};

  static constexpr auto unquote = [](std::string_view t) {
    return t.substr(1, t.size() - 2);
  };
  struct STRING_LITERAL
      : terminal<as<unquote>(lit<"\""> + many(cr<R"(^")">) + lit<"\"">)> {};

  // The fractional part is digits, not a repeated signed integer run: read
  // as the decimal number this spelling names -- every corpus value is a
  // plain (optionally signed) decimal.
  struct DECIMAL_LITERAL : terminal<as<double>(option(cr<"+-">) + some(d) +
                                               option(lit<"."> + many(d)))> {};

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  static constexpr auto arg = append<&ast::Form::args>;

#define EDIF_FORM0(Name, Kw)                                          \
  struct Name : rule<assign<&ast::Form::keyword>(kw<Kw>) + lit<")">> {}
#define EDIF_FORM(Name, Kw, Body)                                \
  struct Name                                                    \
      : rule<assign<&ast::Form::keyword>(kw<Kw>) + (Body) + lit<")">> {}

  // Forward declarations: every construct may reference any other.

  struct Absolute;
  struct AcLoad;
  struct AcLoadDisplay;
  struct AddDisplay;
  struct Ampere;
  struct And;
  struct Annotate;
  struct ApprovedDate;
  struct ApprovedDateDisplay;
  struct Arc;
  struct Ascii;
  struct AssociatedInterconnectNameDisplay;
  struct Author;
  struct BackgroundColor;
  struct BaselineJustify;
  struct Becomes;
  struct BehaviorView;
  struct Bidirectional;
  struct BidirectionalPort;
  struct BidirectionalPortAttributes;
  struct BitOrder;
  struct BoldStyle;
  struct Eboolean;
  struct BooleanConstant;
  struct BooleanConstantRef;
  struct BooleanMap;
  struct BooleanParameter;
  struct BooleanParameterAssign;
  struct BooleanParameterRef;
  struct BorderPattern;
  struct BorderPatternVisible;
  struct BorderWidth;
  struct BottomJustify;
  struct Calculated;
  struct Candela;
  struct CaplineJustify;
  struct Cell;
  struct CellHeader;
  struct CellNameDisplay;
  struct CellPropertyDisplay;
  struct CellPropertyDisplayOverride;
  struct CellPropertyOverride;
  struct CellRef;
  struct Celsius;
  struct CenterJustify;
  struct CharacterEncoding;
  struct CheckDate;
  struct CheckDateDisplay;
  struct Circle;
  struct Cluster;
  struct ClusterConfiguration;
  struct ClusterConfigurationNameCaseSensitive;
  struct ClusterConfigurationRef;
  struct ClusterHeader;
  struct ClusterNameCaseSensitive;
  struct ClusterPropertyDisplay;
  struct ClusterPropertyDisplayOverride;
  struct ClusterPropertyOverride;
  struct ClusterRef;
  struct Color;
  struct Comment;
  struct CommentGraphics;
  struct CompanyName;
  struct CompanyNameDisplay;
  struct ComplementedName;
  struct ComplementedNamePart;
  struct ComplexGeometry;
  struct ComplexName;
  struct Compound;
  struct Condition;
  struct ConditionDisplay;
  struct ConnectedSignalIndexGenerator;
  struct ConnectedSignalIndexGeneratorDisplay;
  struct ConnectivityBus;
  struct ConnectivityBusJoined;
  struct ConnectivityBusSlice;
  struct ConnectivityFrameStructure;
  struct ConnectivityNet;
  struct ConnectivityNetJoined;
  struct ConnectivityRipper;
  struct ConnectivityRipperRef;
  struct ConnectivityStructure;
  struct ConnectivitySubBus;
  struct ConnectivitySubNet;
  struct ConnectivityTagGenerator;
  struct ConnectivityTagGeneratorDisplay;
  struct ConnectivityUnits;
  struct ConnectivityView;
  struct ConnectivityViewHeader;
  struct ConstantValues;
  struct Contract;
  struct ContractDisplay;
  struct Copyright;
  struct CopyrightDisplay;
  struct CornerType;
  struct Coulomb;
  struct Criticality;
  struct CriticalityDisplay;
  struct CurrentMap;
  struct Curve;
  struct DataOrigin;
  struct Date;
  struct DcFanInLoad;
  struct DcFanInLoadDisplay;
  struct DcFanOutLoad;
  struct DcFanOutLoadDisplay;
  struct DcMaxFanIn;
  struct DcMaxFanInDisplay;
  struct DcMaxFanOut;
  struct DcMaxFanOutDisplay;
  struct DecimalToString;
  struct DefaultClusterConfiguration;
  struct DefaultConnection;
  struct Degree;
  struct Delay;
  struct DerivedFrom;
  struct Design;
  struct Designator;
  struct DesignatorDisplay;
  struct DesignHeader;
  struct DesignHierarchy;
  struct DesignHierarchyHeader;
  struct DesignHierarchyNameCaseSensitive;
  struct DesignNameCaseSensitive;
  struct DesignUnits;
  struct Diagram;
  struct DirectionalPortAttributeOverride;
  struct Display;
  struct DisplayAttributes;
  struct DisplayName;
  struct DisplayNameOverride;
  struct Documentation;
  struct DocumentationHeader;
  struct DocumentationNameCaseSensitive;
  struct DocumentationUnits;
  struct Dominates;
  struct Dot;
  struct DrawingDescription;
  struct DrawingDescriptionDisplay;
  struct DrawingIdentification;
  struct DrawingIdentificationDisplay;
  struct DrawingSize;
  struct DrawingSizeDisplay;
  struct Duration;
  struct E;
  struct Edif;
  struct EdifHeader;
  struct EdifLevel;
  struct EdifVersion;
  struct EndType;
  struct EngineeringDate;
  struct EngineeringDateDisplay;
  struct Event;
  struct Extend;
  struct External;
  struct Fahrenheit;
  struct Efalse;
  struct Farad;
  struct Figure;
  struct FigureGroup;
  struct FigureGroupNameCaseSensitive;
  struct FigureGroupOverride;
  struct FillPattern;
  struct FillPatternVisible;
  struct Fixed;
  struct Font;
  struct FontCapitalHeight;
  struct FontDefinitions;
  struct FontDescent;
  struct FontHeight;
  struct FontProportions;
  struct FontRef;
  struct Fonts;
  struct FontWidth;
  struct ForbiddenEvent;
  struct ForFrame;
  struct ForFrameAnnotate;
  struct ForFrameIndex;
  struct ForFrameIndexDisplay;
  struct ForFrameIndexNameCaseSensitive;
  struct ForFrameIndexRef;
  struct ForFrameMemberRef;
  struct ForFrameRef;
  struct FrameConfiguration;
  struct FrameNameCaseSensitive;
  struct FrameRef;
  struct FromBottom;
  struct FromLeft;
  struct FromRight;
  struct FromTop;
  struct Generated;
  struct GeometryMacro;
  struct GeometryMacroHeader;
  struct GeometryMacroRef;
  struct GeometryMacroUnits;
  struct GlobalPort;
  struct GlobalPortBundle;
  struct GlobalPortDefinitions;
  struct GlobalPortList;
  struct GlobalPortNameCaseSensitive;
  struct GlobalPortNameDisplay;
  struct GlobalPortPropertyDisplay;
  struct GlobalPortRef;
  struct GlobalPortScope;
  struct Henry;
  struct Hertz;
  struct HorizontalJustification;
  struct Hotspot;
  struct HotspotConnectDirection;
  struct HotspotGrid;
  struct HotspotNameCaseSensitive;
  struct HotspotNameDisplay;
  struct IeeeSection;
  struct IeeeStandard;
  struct IfFrame;
  struct IfFrameAnnotate;
  struct IfFrameRef;
  struct IfFrameSet;
  struct Ignore;
  struct ImplementationNameCaseSensitive;
  struct ImplementationNameDisplay;
  struct IndexEndDisplay;
  struct IndexNameDisplay;
  struct IndexStart;
  struct IndexStartDisplay;
  struct IndexStep;
  struct IndexStepDisplay;
  struct IndexValue;
  struct Input;
  struct InputPort;
  struct InputPortAttributes;
  struct Instance;
  struct InstanceConfiguration;
  struct InstanceIndexValue;
  struct InstanceMemberRef;
  struct InstanceNameCaseSensitive;
  struct InstanceNameDisplay;
  struct InstanceNameGenerator;
  struct InstanceNameGeneratorDisplay;
  struct InstancePortAttributeDisplay;
  struct InstancePortAttributes;
  struct InstancePropertyDisplay;
  struct InstancePropertyOverride;
  struct InstanceRef;
  struct InstanceWidth;
  struct InstanceWidthDisplay;
  struct Integer;
  struct IntegerConstant;
  struct IntegerConstantRef;
  struct IntegerEqual;
  struct IntegerParameter;
  struct IntegerParameterAssign;
  struct IntegerParameterRef;
  struct IntegerProduct;
  struct IntegerQuotient;
  struct IntegerRemainder;
  struct IntegerSubtract;
  struct IntegerSum;
  struct InterconnectAnnotate;
  struct InterconnectAttachedText;
  struct InterconnectDelay;
  struct InterconnectDelayDisplay;
  struct InterconnectHeader;
  struct InterconnectNameCaseSensitive;
  struct InterconnectNameDisplay;
  struct InterconnectPropertyDisplay;
  struct InterconnectRef;
  struct InterconnectSet;
  struct Einterface;
  struct InterfaceJoined;
  struct InterfaceUnits;
  struct Iso8859;
  struct Isolated;
  struct ItalicStyle;
  struct Jisx0201;
  struct Jisx0208;
  struct JoinedAsSignal;
  struct Joule;
  struct K0KeywordLevel;
  struct K1KeywordAlias;
  struct K1KeywordLevel;
  struct K2Actual;
  struct K2Build;
  struct K2Formal;
  struct K2Generate;
  struct K2KeywordDefine;
  struct K2KeywordLevel;
  struct K2KeywordParameters;
  struct K2Literal;
  struct K2Optional;
  struct K2Required;
  struct K3Build;
  struct K3Collector;
  struct K3ForEach;
  struct K3Formal;
  struct K3FormalList;
  struct K3Generate;
  struct K3KeywordDefine;
  struct K3KeywordLevel;
  struct K3KeywordParameters;
  struct Kelvin;
  struct KeywordMap;
  struct Kilogram;
  struct Leaf;
  struct LeafOccurrenceAnnotate;
  struct LeftJustify;
  struct LessThan;
  struct LessThanOrEqual;
  struct Library;
  struct LibraryHeader;
  struct LibraryNameCaseSensitive;
  struct LibraryObjectNameCaseSensitive;
  struct LibraryRef;
  struct LoadDelay;
  struct LocalPortGroup;
  struct LocalPortGroupNameCaseSensitive;
  struct LocalPortGroupRef;
  struct LogicalConnectivity;
  struct LogicDefinitions;
  struct LogicList;
  struct LogicMapInput;
  struct LogicMapOutput;
  struct LogicModelUnits;
  struct LogicModelView;
  struct LogicOneOf;
  struct LogicRef;
  struct LogicValue;
  struct LsbToMsb;
  struct MaskLayoutUnits;
  struct MaskLayoutView;
  struct Measured;
  struct Meter;
  struct MiddleJustify;
  struct MinimalWidth;
  struct MinimumStringLength;
  struct MiNoMax;
  struct MixedDirection;
  struct Mnm;
  struct Mole;
  struct MsbToLsb;
  struct MustJoin;
  struct NameAlias;
  struct NameCaseSensitivity;
  struct NameDimension;
  struct NameDimensionStructure;
  struct NameInformation;
  struct NamePartSeparator;
  struct NameStructure;
  struct NarrowPort;
  struct NarrowWire;
  struct NoHotspotGrid;
  struct NominalHotspotGrid;
  struct NonPermutable;
  struct Not;
  struct NotInherited;
  struct Number;
  struct NumberParameter;
  struct NumberParameterAssign;
  struct NumberParameterRef;
  struct NumberPoint;
  struct OccurrenceAnnotate;
  struct OccurrenceHierarchyAnnotate;
  struct OffsetEvent;
  struct Ohm;
  struct OpenShape;
  struct Or;
  struct Origin;
  struct OriginalBoundingBox;
  struct OriginalDrawingDate;
  struct OriginalDrawingDateDisplay;
  struct OtherwiseFrame;
  struct OtherwiseFrameAnnotate;
  struct OtherwiseFrameRef;
  struct Output;
  struct OutputPort;
  struct OutputPortAttributes;
  struct Owner;
  struct Page;
  struct PageAnnotate;
  struct PageBorder;
  struct PageBorderTemplate;
  struct PageBorderTemplateRef;
  struct PageCommentGraphics;
  struct PageHeader;
  struct PageIdentification;
  struct PageIdentificationDisplay;
  struct PageNameCaseSensitive;
  struct PagePropertyDisplay;
  struct PageRef;
  struct PageSize;
  struct PageTitle;
  struct PageTitleBlock;
  struct PageTitleBlockAttributeDisplay;
  struct PageTitleBlockAttributes;
  struct PageTitleBlockTemplate;
  struct PageTitleBlockTemplateRef;
  struct PageTitleDisplay;
  struct ParameterDisplay;
  struct ParameterNameCaseSensitive;
  struct ParameterNameDisplay;
  struct Path;
  struct PathDelay;
  struct PathWidth;
  struct PcbLayoutUnits;
  struct PcbLayoutView;
  struct Permutable;
  struct PhysicalDefaults;
  struct PhysicalScaling;
  struct PixelPattern;
  struct PixelRow;
  struct Point;
  struct PointList;
  struct Polygon;
  struct Port;
  struct PortAnnotate;
  struct PortAttributeDisplay;
  struct PortBundle;
  struct PortDelay;
  struct PortDelayDisplay;
  struct PortDelayOverride;
  struct PortIndexValue;
  struct PortInstanceRef;
  struct PortJoined;
  struct PortList;
  struct PortLoadDelay;
  struct PortLoadDelayDisplay;
  struct PortLoadDelayOverride;
  struct PortMemberRef;
  struct PortNameCaseSensitive;
  struct PortNameDisplay;
  struct PortNameGenerator;
  struct PortNameGeneratorDisplay;
  struct PortPropertyDisplay;
  struct PortPropertyDisplayOverride;
  struct PortPropertyOverride;
  struct PortRef;
  struct PortSet;
  struct PortWidth;
  struct PreviousVersion;
  struct PrimaryName;
  struct Program;
  struct Property;
  struct PropertyDisplay;
  struct PropertyDisplayOverride;
  struct PropertyInheritanceControl;
  struct PropertyNameCaseSensitive;
  struct PropertyNameDisplay;
  struct PropertyOverride;
  struct ProportionalFont;
  struct Pt;
  struct Radian;
  struct Reason;
  struct Rectangle;
  struct RemoveDisplay;
  struct RepetitionCount;
  struct RepetitionCountDisplay;
  struct ReplaceDisplay;
  struct Required;
  struct Resolves;
  struct Revision;
  struct RevisionDisplay;
  struct RightJustify;
  struct RipperHotspot;
  struct RipperHotspotRef;
  struct Rotation;
  struct Round;
  struct ScaleX;
  struct ScaleY;
  struct SchematicBus;
  struct SchematicBusDetails;
  struct SchematicBusGraphics;
  struct SchematicBusJoined;
  struct SchematicBusSlice;
  struct SchematicComplexFigure;
  struct SchematicFigureMacro;
  struct SchematicFigureMacroRef;
  struct SchematicForFrameBorder;
  struct SchematicForFrameBorderTemplate;
  struct SchematicForFrameBorderTemplateRef;
  struct SchematicForFrameImplementation;
  struct SchematicForFrameImplementationHeader;
  struct SchematicFrameImplementationDetails;
  struct SchematicGlobalPortAttributes;
  struct SchematicGlobalPortImplementation;
  struct SchematicGlobalPortImplementationRef;
  struct SchematicGlobalPortTemplate;
  struct SchematicGlobalPortTemplateRef;
  struct SchematicIfFrameBorder;
  struct SchematicIfFrameBorderTemplate;
  struct SchematicIfFrameBorderTemplateRef;
  struct SchematicIfFrameImplementation;
  struct SchematicIfFrameImplementationHeader;
  struct SchematicImplementation;
  struct SchematicInstanceImplementation;
  struct SchematicInstanceImplementationRef;
  struct SchematicInterconnectAttributeDisplay;
  struct SchematicInterconnectHeader;
  struct SchematicInterconnectTerminatorImplementation;
  struct SchematicInterconnectTerminatorImplementationRef;
  struct SchematicInterconnectTerminatorTemplate;
  struct SchematicInterconnectTerminatorTemplateRef;
  struct SchematicJunctionImplementation;
  struct SchematicJunctionImplementationRef;
  struct SchematicJunctionTemplate;
  struct SchematicJunctionTemplateRef;
  struct SchematicMasterPortImplementation;
  struct SchematicMasterPortImplementationRef;
  struct SchematicMasterPortTemplate;
  struct SchematicMasterPortTemplateRef;
  struct SchematicMetric;
  struct SchematicNet;
  struct SchematicNetDetails;
  struct SchematicNetGraphics;
  struct SchematicNetJoined;
  struct SchematicOffPageConnectorImplementation;
  struct SchematicOffPageConnectorImplementationRef;
  struct SchematicOffPageConnectorTemplate;
  struct SchematicOffPageConnectorTemplateRef;
  struct SchematicOnPageConnectorImplementation;
  struct SchematicOnPageConnectorImplementationRef;
  struct SchematicOnPageConnectorTemplate;
  struct SchematicOnPageConnectorTemplateRef;
  struct SchematicOtherwiseFrameBorder;
  struct SchematicOtherwiseFrameBorderTemplate;
  struct SchematicOtherwiseFrameBorderTemplateRef;
  struct SchematicOtherwiseFrameImplementation;
  struct SchematicOtherwiseFrameImplementationHeader;
  struct SchematicPortAcPower;
  struct SchematicPortAcPowerRecommendedFrequency;
  struct SchematicPortAcPowerRecommendedVoltageRms;
  struct SchematicPortAnalog;
  struct SchematicPortAttributes;
  struct SchematicPortChassisGround;
  struct SchematicPortClock;
  struct SchematicPortDcPower;
  struct SchematicPortDcPowerRecommendedVoltage;
  struct SchematicPortEarthGround;
  struct SchematicPortGround;
  struct SchematicPortNonLogical;
  struct SchematicPortOpenCollector;
  struct SchematicPortOpenEmitter;
  struct SchematicPortPower;
  struct SchematicPortStyle;
  struct SchematicPortThreeState;
  struct SchematicRequiredDefaults;
  struct SchematicRipperImplementation;
  struct SchematicRipperImplementationRef;
  struct SchematicRipperTemplate;
  struct SchematicRipperTemplateRef;
  struct SchematicSubBus;
  struct SchematicSubBusSet;
  struct SchematicSubInterconnectHeader;
  struct SchematicSubNet;
  struct SchematicSubNetSet;
  struct SchematicSymbol;
  struct SchematicSymbolBorder;
  struct SchematicSymbolBorderTemplate;
  struct SchematicSymbolBorderTemplateRef;
  struct SchematicSymbolHeader;
  struct SchematicSymbolPortImplementation;
  struct SchematicSymbolPortImplementationRef;
  struct SchematicSymbolPortTemplate;
  struct SchematicSymbolPortTemplateRef;
  struct SchematicSymbolRef;
  struct SchematicTemplateHeader;
  struct SchematicUnits;
  struct SchematicView;
  struct SchematicViewHeader;
  struct SchematicWireAffinity;
  struct SchematicWireStyle;
  struct Second;
  struct Section;
  struct SectionTitle;
  struct Sequence;
  struct SetAngle;
  struct SetCapacitance;
  struct SetCurrent;
  struct SetDistance;
  struct SetFrequency;
  struct SetTime;
  struct SetVoltage;
  struct Shape;
  struct Siemens;
  struct Signal;
  struct SignalAnnotate;
  struct SignalGroup;
  struct SignalGroupAnnotate;
  struct SignalGroupNameCaseSensitive;
  struct SignalGroupRef;
  struct SignalIndexValue;
  struct SignalJoined;
  struct SignalList;
  struct SignalMemberRef;
  struct SignalNameCaseSensitive;
  struct SignalRef;
  struct SignalWidth;
  struct Status;
  struct Step;
  struct String;
  struct StringConcatenate;
  struct StringConstant;
  struct StringConstantRef;
  struct StringEqual;
  struct StringLength;
  struct StringParameter;
  struct StringParameterAssign;
  struct StringParameterRef;
  struct StringPrefix;
  struct StringSuffix;
  struct Strong;
  struct Substring;
  struct SymbolicLayoutUnits;
  struct SymbolicLayoutView;
  struct Technology;
  struct TextHeight;
  struct Time;
  struct TimeInterval;
  struct TimeStamp;
  struct Timing;
  struct TimingDisplay;
  struct TopJustify;
  struct TotalPages;
  struct TotalPagesDisplay;
  struct Transform;
  struct Transition;
  struct Etrue;
  struct Truncate;
  struct Typeface;
  struct TypefaceSuffix;
  struct Unconfigured;
  struct Unconstrained;
  struct Undefined;
  struct Unit;
  struct UnitDefinitions;
  struct UnitRef;
  struct Unrestricted;
  struct Unspecified;
  struct UnspecifiedDirectionPort;
  struct Untyped;
  struct Unused;
  struct UsableArea;
  struct UserData;
  struct Version;
  struct VerticalJustification;
  struct ViewGroup;
  struct ViewGroupHeader;
  struct ViewGroupNameCaseSensitive;
  struct ViewGroupRef;
  struct ViewNameCaseSensitive;
  struct ViewNameDisplay;
  struct ViewPropertyDisplay;
  struct ViewPropertyOverride;
  struct ViewRef;
  struct Visible;
  struct Volt;
  struct VoltageMap;
  struct Watt;
  struct Weak;
  struct WeakJoined;
  struct Weber;
  struct WidePort;
  struct WideWire;
  struct Written;
  struct Xor;
  struct Year;

  // Type-union / pure-forwarding rules: shared expressions, not nodes

  // of their own -- spliced wherever the construct they stand for is named.

  static constexpr auto integerTokenExpr = ref<DECIMAL_LITERAL>;
  static constexpr auto scaledIntegerExpr = integerTokenExpr | ref<E>;
  static constexpr auto numberValueExpr = scaledIntegerExpr;
  static constexpr auto miNoMaxValueExpr = numberValueExpr | ref<Mnm>;
  static constexpr auto capacitanceValueExpr = miNoMaxValueExpr;
  static constexpr auto acLoadFactorCapacitanceExpr = capacitanceValueExpr;
  static constexpr auto timeValueExpr = miNoMaxValueExpr;
  static constexpr auto acLoadFactorTimeExpr = timeValueExpr;
  static constexpr auto angleValueExpr = numberValueExpr;
  static constexpr auto pointValueExpr = ref<Pt>;
  static constexpr auto attachmentPointExpr = pointValueExpr;
  static constexpr auto blueExpr = scaledIntegerExpr;
  static constexpr auto booleanTokenExpr = ref<Efalse> | ref<Etrue>;
  static constexpr auto booleanExpressionExpr = ref<And> | ref<BooleanParameterRef> | booleanTokenExpr | ref<StringEqual> | ref<IntegerEqual> | ref<LessThan> | ref<LessThanOrEqual> | ref<Not> | ref<Or> | ref<Xor> | ref<BooleanConstantRef>;
  static constexpr auto booleanValueExpr = booleanTokenExpr;
  static constexpr auto nameDefExpr = ref<IDENTIFIER>;
  static constexpr auto clusterConfigurationNameDefExpr = nameDefExpr;
  static constexpr auto nameRefExpr = ref<IDENTIFIER>;
  static constexpr auto clusterConfigurationNameRefExpr = nameRefExpr;
  static constexpr auto clusterNameDefExpr = nameDefExpr;
  static constexpr auto clusterNameRefExpr = nameRefExpr;
  static constexpr auto connectivityFrameStructureNameDefExpr = nameDefExpr;
  static constexpr auto connectivityRipperNameDefExpr = nameDefExpr;
  static constexpr auto connectivityRipperNameRefExpr = nameRefExpr;
  static constexpr auto constantNameDefExpr = nameDefExpr;
  static constexpr auto constantNameRefExpr = nameRefExpr;
  static constexpr auto currentValueExpr = miNoMaxValueExpr;
  static constexpr auto dayNumberExpr = integerTokenExpr;
  static constexpr auto integerValueExpr = integerTokenExpr;
  static constexpr auto denominatorExpr = integerValueExpr;
  static constexpr auto derivationExpr = ref<Calculated> | ref<Measured> | ref<Required>;
  static constexpr auto designHierarchyNameDefExpr = nameDefExpr;
  static constexpr auto designNameDefExpr = nameDefExpr;
  static constexpr auto diagramNameDefExpr = nameDefExpr;
  static constexpr auto distanceValueExpr = integerValueExpr;
  static constexpr auto integerExpressionExpr = ref<IntegerParameterRef> | integerTokenExpr | ref<IntegerProduct> | ref<IntegerSubtract> | ref<IntegerSum> | ref<IntegerRemainder> | ref<IntegerQuotient> | ref<StringLength> | ref<IntegerConstantRef> | ref<ForFrameIndexRef> | ref<PortIndexValue> | ref<SignalIndexValue> | ref<Absolute> | ref<InstanceIndexValue>;
  static constexpr auto dividendExpr = integerExpressionExpr;
  static constexpr auto divisorExpr = integerExpressionExpr;
  static constexpr auto documentationNameDefExpr = nameDefExpr;
  static constexpr auto edifLevelValueExpr = integerTokenExpr;
  static constexpr auto edifNameDefExpr = nameDefExpr;
  static constexpr auto endPointExpr = pointValueExpr;
  static constexpr auto exponentExpr = integerTokenExpr;
  static constexpr auto extendForFrameMemberDefExpr = ref<ForFrameMemberRef>;
  static constexpr auto frameNameRefExpr = nameRefExpr;
  static constexpr auto extendFrameDefExpr = frameNameRefExpr;
  static constexpr auto instanceNameRefExpr = nameRefExpr;
  static constexpr auto extendInstanceDefExpr = instanceNameRefExpr;
  static constexpr auto extendInstanceMemberDefExpr = ref<InstanceMemberRef>;
  static constexpr auto interconnectNameRefExpr = nameRefExpr;
  static constexpr auto extendInterconnectDefExpr = interconnectNameRefExpr;
  static constexpr auto pageNameRefExpr = nameRefExpr;
  static constexpr auto extendPageDefExpr = pageNameRefExpr;
  static constexpr auto portNameRefExpr = nameRefExpr;
  static constexpr auto extendPortDefExpr = portNameRefExpr;
  static constexpr auto extendPortMemberDefExpr = ref<PortMemberRef>;
  static constexpr auto signalNameRefExpr = nameRefExpr;
  static constexpr auto extendSignalDefExpr = signalNameRefExpr;
  static constexpr auto signalGroupNameRefExpr = nameRefExpr;
  static constexpr auto extendSignalGroupDefExpr = signalGroupNameRefExpr;
  static constexpr auto extendSignalMemberDefExpr = ref<SignalMemberRef>;
  static constexpr auto figureGroupNameDefExpr = nameDefExpr;
  static constexpr auto figureGroupNameRefExpr = nameRefExpr;
  static constexpr auto firstIntegerExpressionExpr = integerExpressionExpr;
  static constexpr auto stringTokenExpr = ref<STRING_LITERAL>;
  static constexpr auto stringExpressionExpr = ref<StringParameterRef> | stringTokenExpr | ref<StringConstantRef> | ref<StringConcatenate> | ref<Substring> | ref<DecimalToString> | ref<StringPrefix> | ref<StringSuffix>;
  static constexpr auto firstStringExpressionExpr = stringExpressionExpr;
  static constexpr auto fontFamilyExpr = stringTokenExpr;
  static constexpr auto fontNameDefExpr = nameDefExpr;
  static constexpr auto fontNameRefExpr = nameRefExpr;
  static constexpr auto frameNameDefExpr = nameDefExpr;
  static constexpr auto frequencyValueExpr = miNoMaxValueExpr;
  static constexpr auto fromIntegerExpr = integerTokenExpr;
  static constexpr auto globalPortNameDefExpr = nameDefExpr;
  static constexpr auto globalPortNameRefExpr = nameRefExpr;
  static constexpr auto greenExpr = scaledIntegerExpr;
  static constexpr auto hotspotNameDefExpr = nameDefExpr;
  static constexpr auto hotspotNameRefExpr = nameRefExpr;
  static constexpr auto hourNumberExpr = integerTokenExpr;
  static constexpr auto ieeeDesignationExpr = stringTokenExpr;
  static constexpr auto implementationNameDefExpr = nameDefExpr;
  static constexpr auto implementationNameRefExpr = nameRefExpr;
  static constexpr auto indexNameDefExpr = nameDefExpr;
  static constexpr auto indexNameRefExpr = nameRefExpr;
  static constexpr auto instanceNameDefExpr = nameDefExpr;
  static constexpr auto interconnectDelayNameDefExpr = nameDefExpr;
  static constexpr auto interconnectDelayNameRefExpr = nameRefExpr;
  static constexpr auto interconnectNameDefExpr = nameDefExpr;
  static constexpr auto iso8859PartExpr = integerTokenExpr;
  static constexpr auto issueExpr = integerTokenExpr;
  static constexpr auto k1KeywordNameDefExpr = ref<IDENTIFIER>;
  static constexpr auto k1KeywordNameRefExpr = ref<IDENTIFIER>;
  static constexpr auto k2FormalNameDefExpr = ref<IDENTIFIER>;
  static constexpr auto k2FormalNameRefExpr = ref<IDENTIFIER>;
  static constexpr auto lengthValueExpr = distanceValueExpr;
  static constexpr auto libraryNameDefExpr = nameDefExpr;
  static constexpr auto libraryNameRefExpr = nameRefExpr;
  static constexpr auto libraryObjectNameDefExpr = nameDefExpr;
  static constexpr auto libraryObjectNameRefExpr = nameRefExpr;
  static constexpr auto localPortGroupNameDefExpr = nameDefExpr;
  static constexpr auto localPortGroupNameRefExpr = nameRefExpr;
  static constexpr auto logicNameDefExpr = nameDefExpr;
  static constexpr auto logicNameRefExpr = nameRefExpr;
  static constexpr auto mantissaExpr = integerTokenExpr;
  static constexpr auto markExpr = integerTokenExpr;
  static constexpr auto minuendExpr = integerExpressionExpr;
  static constexpr auto minuteNumberExpr = integerTokenExpr;
  static constexpr auto monthNumberExpr = integerTokenExpr;
  static constexpr auto numberExpressionExpr = numberValueExpr | ref<NumberParameterRef>;
  static constexpr auto numberOfBasicUnitsExpr = scaledIntegerExpr;
  static constexpr auto numberOfNewUnitsExpr = scaledIntegerExpr;
  static constexpr auto numeratorExpr = integerValueExpr;
  static constexpr auto originalNameExpr = stringTokenExpr;
  static constexpr auto pageNameDefExpr = nameDefExpr;
  static constexpr auto parameterNameDefExpr = nameDefExpr;
  static constexpr auto parameterNameRefExpr = nameRefExpr;
  static constexpr auto portDelayNameDefExpr = nameDefExpr;
  static constexpr auto portDelayNameRefExpr = nameRefExpr;
  static constexpr auto portDirectionExpr = ref<InputPort> | ref<OutputPort> | ref<BidirectionalPort> | ref<UnspecifiedDirectionPort>;
  static constexpr auto portDirectionIndicatorExpr = ref<Input> | ref<Output> | ref<Bidirectional> | ref<Unspecified> | ref<Unrestricted> | ref<MixedDirection>;
  static constexpr auto portLoadDelayNameDefExpr = nameDefExpr;
  static constexpr auto portLoadDelayNameRefExpr = nameRefExpr;
  static constexpr auto portNameDefExpr = nameDefExpr;
  static constexpr auto presentLogicValueExpr = logicNameRefExpr | ref<LogicList> | ref<LogicOneOf>;
  static constexpr auto previousLogicValueExpr = logicNameRefExpr | ref<LogicList> | ref<LogicOneOf>;
  static constexpr auto propertyNameDefExpr = nameDefExpr;
  static constexpr auto propertyNameRefExpr = nameRefExpr;
  static constexpr auto pt1Expr = pointValueExpr;
  static constexpr auto pt2Expr = pointValueExpr;
  static constexpr auto redExpr = scaledIntegerExpr;
  static constexpr auto rowSizeExpr = integerTokenExpr;
  static constexpr auto secondIntegerExpressionExpr = integerExpressionExpr;
  static constexpr auto secondNumberExpr = integerTokenExpr;
  static constexpr auto secondStringExpressionExpr = stringExpressionExpr;
  static constexpr auto sectionNameDefExpr = nameDefExpr;
  static constexpr auto signalGroupNameDefExpr = nameDefExpr;
  static constexpr auto signalNameDefExpr = nameDefExpr;
  static constexpr auto simpleNameExpr = stringTokenExpr;
  static constexpr auto startPointExpr = pointValueExpr;
  static constexpr auto stringValueExpr = stringTokenExpr;
  static constexpr auto subIssueExpr = integerTokenExpr;
  static constexpr auto substringLengthExpr = integerExpressionExpr;
  static constexpr auto substringOffsetExpr = integerExpressionExpr;
  static constexpr auto subtrahendExpr = integerExpressionExpr;
  static constexpr auto symbolPortImplementationNameDefExpr = nameDefExpr;
  static constexpr auto symbolPortImplementationNameRefExpr = nameRefExpr;
  static constexpr auto throughPointExpr = ref<NumberPoint>;
  static constexpr auto timeDelayExpr = timeValueExpr;
  static constexpr auto timingNameDefExpr = nameDefExpr;
  static constexpr auto timingNameRefExpr = nameRefExpr;
  static constexpr auto toIntegerExpr = integerTokenExpr;
  static constexpr auto typedValueExpr = ref<Eboolean> | ref<Integer> | ref<MiNoMax> | ref<Number> | ref<Point> | ref<String>;
  static constexpr auto unitExponentExpr = scaledIntegerExpr;
  static constexpr auto unitNameDefExpr = nameDefExpr;
  static constexpr auto unitNameRefExpr = nameRefExpr;
  static constexpr auto userDataTagExpr = ref<IDENTIFIER>;
  static constexpr auto viewGroupNameDefExpr = nameDefExpr;
  static constexpr auto viewGroupNameRefExpr = nameRefExpr;
  static constexpr auto viewNameDefExpr = nameDefExpr;
  static constexpr auto viewNameRefExpr = nameRefExpr;
  static constexpr auto voltageValueExpr = miNoMaxValueExpr;
  static constexpr auto xCoordinateExpr = integerValueExpr;
  static constexpr auto xNumberValueExpr = numberValueExpr;
  static constexpr auto yCoordinateExpr = integerValueExpr;
  static constexpr auto yearNumberExpr = integerTokenExpr;
  static constexpr auto yNumberValueExpr = numberValueExpr;

  // Every remaining construct: one macro call each, alphabetical by name.

  EDIF_FORM(Absolute, "(absolute",
           arg(integerExpressionExpr));
  EDIF_FORM(AcLoad, "(acLoad",
           arg(capacitanceValueExpr));
  EDIF_FORM(AcLoadDisplay, "(acLoadDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(AddDisplay, "(addDisplay",
           many(arg(ref<Display>)));
  EDIF_FORM(Ampere, "(ampere",
           arg(unitExponentExpr));
  EDIF_FORM(And, "(and",
           many(arg(booleanExpressionExpr)));
  EDIF_FORM(Annotate, "(annotate",
           arg(stringValueExpr) +
           many(arg(ref<Display>)));
  EDIF_FORM(ApprovedDate, "(approvedDate",
           arg(ref<Date>));
  EDIF_FORM(ApprovedDateDisplay, "(approvedDateDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(Arc, "(arc",
           arg(startPointExpr) +
           arg(throughPointExpr) +
           arg(endPointExpr));
  EDIF_FORM0(Ascii, "(ascii");
  EDIF_FORM(AssociatedInterconnectNameDisplay, "(associatedInterconnectNameDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(Author, "(author",
           arg(stringTokenExpr));
  EDIF_FORM(BackgroundColor, "(backgroundColor",
           arg(ref<Color>));
  EDIF_FORM0(BaselineJustify, "(baselineJustify");
  EDIF_FORM(Becomes, "(becomes",
           arg(logicNameRefExpr | ref<LogicList> | ref<LogicOneOf>));
  EDIF_FORM(BehaviorView, "(behaviorView",
           arg(viewNameDefExpr) +
           many(arg(ref<Comment> | ref<NameInformation> | ref<UserData>)));
  EDIF_FORM0(Bidirectional, "(bidirectional");
  EDIF_FORM(BidirectionalPort, "(bidirectionalPort",
           option(arg(ref<BidirectionalPortAttributes>)));
  EDIF_FORM(BidirectionalPortAttributes, "(bidirectionalPortAttributes",
           many(arg(ref<DcFanInLoad> | ref<DcFanOutLoad> | ref<DcMaxFanIn> | ref<DcMaxFanOut>)));
  EDIF_FORM(BitOrder, "(bitOrder",
           arg(ref<LsbToMsb> | ref<MsbToLsb>));
  EDIF_FORM0(BoldStyle, "(boldStyle");
  EDIF_FORM(Eboolean, "(boolean",
           arg(booleanExpressionExpr));
  EDIF_FORM(BooleanConstant, "(booleanConstant",
           arg(constantNameDefExpr) +
           arg(booleanTokenExpr));
  EDIF_FORM(BooleanConstantRef, "(booleanConstantRef",
           arg(constantNameRefExpr));
  EDIF_FORM(BooleanMap, "(booleanMap",
           arg(booleanValueExpr));
  EDIF_FORM(BooleanParameter, "(booleanParameter",
           arg(parameterNameDefExpr) +
           many(arg(ref<Eboolean> | ref<NameInformation>)));
  EDIF_FORM(BooleanParameterAssign, "(booleanParameterAssign",
           arg(parameterNameRefExpr) +
           arg(booleanExpressionExpr));
  EDIF_FORM(BooleanParameterRef, "(booleanParameterRef",
           arg(parameterNameRefExpr));
  EDIF_FORM(BorderPattern, "(borderPattern",
           arg(ref<PixelPattern>));
  EDIF_FORM(BorderPatternVisible, "(borderPatternVisible",
           arg(booleanValueExpr));
  EDIF_FORM(BorderWidth, "(borderWidth",
           arg(lengthValueExpr | ref<MinimalWidth>));
  EDIF_FORM0(BottomJustify, "(bottomJustify");
  EDIF_FORM0(Calculated, "(calculated");
  EDIF_FORM(Candela, "(candela",
           arg(unitExponentExpr));
  EDIF_FORM0(CaplineJustify, "(caplineJustify");
  EDIF_FORM(Cell, "(cell",
           arg(libraryObjectNameDefExpr) +
           arg(ref<CellHeader>) +
           many(arg(ref<Cluster> | ref<Comment> | ref<UserData> | ref<ViewGroup>)));
  EDIF_FORM(CellHeader, "(cellHeader",
           many(arg(ref<Documentation> | ref<NameInformation> | ref<Property> | ref<Status>)));
  EDIF_FORM(CellNameDisplay, "(cellNameDisplay",
           many(arg(ref<Display> | ref<DisplayNameOverride>)));
  EDIF_FORM(CellPropertyDisplay, "(cellPropertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<Display> | ref<PropertyNameDisplay>)));
  EDIF_FORM(CellPropertyDisplayOverride, "(cellPropertyDisplayOverride",
           arg(propertyNameRefExpr) +
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>) +
           option(arg(ref<PropertyNameDisplay>)));
  EDIF_FORM(CellPropertyOverride, "(cellPropertyOverride",
           arg(propertyNameRefExpr) +
           arg(typedValueExpr | ref<Untyped>) +
           many(arg(ref<Comment> | ref<Fixed> | ref<PropertyOverride>)));
  EDIF_FORM(CellRef, "(cellRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(Celsius, "(celsius",
           arg(unitExponentExpr));
  EDIF_FORM0(CenterJustify, "(centerJustify");
  EDIF_FORM(CharacterEncoding, "(characterEncoding",
           arg(ref<Ascii> | ref<Iso8859> | ref<Jisx0201> | ref<Jisx0208>));
  EDIF_FORM(CheckDate, "(checkDate",
           arg(ref<Date>));
  EDIF_FORM(CheckDateDisplay, "(checkDateDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(Circle, "(circle",
           arg(pt1Expr) +
           arg(pt2Expr));
  EDIF_FORM(Cluster, "(cluster",
           arg(clusterNameDefExpr) +
           arg(ref<Einterface>) +
           arg(ref<ClusterHeader>) +
           many(arg(ref<SchematicSymbol> | ref<SchematicView> | ref<BehaviorView> | ref<ClusterConfiguration> | ref<Comment> | ref<ConnectivityView> | ref<DefaultClusterConfiguration> | ref<UserData> | ref<LogicModelView> | ref<MaskLayoutView> | ref<PcbLayoutView> | ref<SymbolicLayoutView>)));
  EDIF_FORM(ClusterConfiguration, "(clusterConfiguration",
           arg(clusterConfigurationNameDefExpr) +
           arg(ref<ViewRef> | ref<Leaf> | ref<Unconfigured>) +
           many(arg(ref<Comment> | ref<FrameConfiguration> | ref<GlobalPortScope> | ref<NameInformation> | ref<InstanceConfiguration> | ref<Property> | ref<UserData>)));
  EDIF_FORM(ClusterConfigurationNameCaseSensitive, "(clusterConfigurationNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(ClusterConfigurationRef, "(clusterConfigurationRef",
           arg(clusterConfigurationNameRefExpr));
  EDIF_FORM(ClusterHeader, "(clusterHeader",
           many(arg(ref<Documentation> | ref<NameInformation> | ref<Property> | ref<Status>)));
  EDIF_FORM(ClusterNameCaseSensitive, "(clusterNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(ClusterPropertyDisplay, "(clusterPropertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<Display> | ref<PropertyNameDisplay>)));
  EDIF_FORM(ClusterPropertyDisplayOverride, "(clusterPropertyDisplayOverride",
           arg(propertyNameRefExpr) +
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>) +
           option(arg(ref<PropertyNameDisplay>)));
  EDIF_FORM(ClusterPropertyOverride, "(clusterPropertyOverride",
           arg(propertyNameRefExpr) +
           arg(typedValueExpr | ref<Untyped>) +
           many(arg(ref<Comment> | ref<Fixed> | ref<PropertyOverride>)));
  EDIF_FORM(ClusterRef, "(clusterRef",
           arg(clusterNameRefExpr) +
           option(arg(ref<CellRef>)));
  EDIF_FORM(Color, "(color",
           arg(redExpr) +
           arg(greenExpr) +
           arg(blueExpr));
  EDIF_FORM(Comment, "(comment",
           many(arg(stringTokenExpr)));
  EDIF_FORM(CommentGraphics, "(commentGraphics",
           many(arg(ref<Annotate> | ref<Comment> | ref<Figure> | ref<OriginalBoundingBox> | ref<PropertyDisplay> | ref<UserData>)));
  EDIF_FORM(CompanyName, "(companyName",
           arg(stringTokenExpr));
  EDIF_FORM(CompanyNameDisplay, "(companyNameDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(ComplementedName, "(complementedName",
           many(arg(ref<ComplementedNamePart> | ref<NameDimension> | ref<NamePartSeparator> | simpleNameExpr)));
  EDIF_FORM(ComplementedNamePart, "(complementedNamePart",
           many(arg(ref<ComplementedNamePart> | ref<NamePartSeparator> | simpleNameExpr)));
  EDIF_FORM(ComplexGeometry, "(complexGeometry",
           arg(ref<GeometryMacroRef>) +
           arg(ref<Transform>));
  EDIF_FORM(ComplexName, "(complexName",
           many(arg(ref<ComplementedNamePart> | ref<NameDimension> | ref<NamePartSeparator> | simpleNameExpr)));
  EDIF_FORM(Compound, "(compound",
           many(arg(logicNameRefExpr)));
  EDIF_FORM(Condition, "(condition",
           arg(booleanExpressionExpr));
  EDIF_FORM(ConditionDisplay, "(conditionDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(ConnectedSignalIndexGenerator, "(connectedSignalIndexGenerator",
           arg(integerExpressionExpr));
  EDIF_FORM(ConnectedSignalIndexGeneratorDisplay, "(connectedSignalIndexGeneratorDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(ConnectivityBus, "(connectivityBus",
           arg(interconnectNameDefExpr) +
           arg(ref<SignalGroupRef>) +
           arg(ref<InterconnectHeader>) +
           arg(ref<ConnectivityBusJoined>) +
           many(arg(ref<Comment> | ref<ConnectivityBusSlice> | ref<ConnectivitySubBus> | ref<UserData>)));
  EDIF_FORM(ConnectivityBusJoined, "(connectivityBusJoined",
           arg(ref<PortJoined>) +
           many(arg(ref<ConnectivityRipperRef>)));
  EDIF_FORM(ConnectivityBusSlice, "(connectivityBusSlice",
           arg(interconnectNameDefExpr) +
           arg(ref<SignalGroupRef>) +
           arg(ref<InterconnectHeader>) +
           arg(ref<ConnectivityBusJoined>) +
           many(arg(ref<Comment> | ref<ConnectivityBusSlice> | ref<ConnectivitySubBus> | ref<UserData>)));
  EDIF_FORM(ConnectivityFrameStructure, "(connectivityFrameStructure",
           arg(connectivityFrameStructureNameDefExpr) +
           arg(ref<FrameRef>) +
           many(arg(ref<Comment> | ref<ConnectivityBus> | ref<ConnectivityFrameStructure> | ref<ConnectivityNet> | ref<ConnectivityRipper> | ref<Timing> | ref<UserData>)));
  EDIF_FORM(ConnectivityNet, "(connectivityNet",
           arg(interconnectNameDefExpr) +
           arg(ref<SignalRef>) +
           arg(ref<InterconnectHeader>) +
           arg(ref<ConnectivityNetJoined>) +
           many(arg(ref<Comment> | ref<ConnectivitySubNet> | ref<UserData>)));
  EDIF_FORM(ConnectivityNetJoined, "(connectivityNetJoined",
           arg(ref<PortJoined> | ref<JoinedAsSignal>) +
           many(arg(ref<ConnectivityRipperRef>)));
  EDIF_FORM(ConnectivityRipper, "(connectivityRipper",
           arg(connectivityRipperNameDefExpr));
  EDIF_FORM(ConnectivityRipperRef, "(connectivityRipperRef",
           arg(connectivityRipperNameRefExpr));
  EDIF_FORM(ConnectivityStructure, "(connectivityStructure",
           many(arg(ref<Comment> | ref<ConnectivityBus> | ref<ConnectivityFrameStructure> | ref<ConnectivityNet> | ref<ConnectivityRipper> | ref<LocalPortGroup> | ref<Timing> | ref<UserData>)));
  EDIF_FORM(ConnectivitySubBus, "(connectivitySubBus",
           arg(interconnectNameDefExpr) +
           arg(ref<InterconnectHeader>) +
           arg(ref<ConnectivityBusJoined>) +
           many(arg(ref<Comment> | ref<ConnectivityBusSlice> | ref<ConnectivitySubBus> | ref<UserData>)));
  EDIF_FORM(ConnectivitySubNet, "(connectivitySubNet",
           arg(interconnectNameDefExpr) +
           arg(ref<InterconnectHeader>) +
           arg(ref<ConnectivityNetJoined>) +
           many(arg(ref<Comment> | ref<ConnectivitySubNet> | ref<UserData>)));
  EDIF_FORM(ConnectivityTagGenerator, "(connectivityTagGenerator",
           arg(stringExpressionExpr));
  EDIF_FORM(ConnectivityTagGeneratorDisplay, "(connectivityTagGeneratorDisplay",
           many(arg(ref<Display>)));
  EDIF_FORM(ConnectivityUnits, "(connectivityUnits",
           option(arg(ref<SetCapacitance> | ref<SetTime>)));
  EDIF_FORM(ConnectivityView, "(connectivityView",
           arg(viewNameDefExpr) +
           arg(ref<ConnectivityViewHeader>) +
           arg(ref<LogicalConnectivity>) +
           arg(ref<ConnectivityStructure>) +
           many(arg(ref<Comment> | ref<UserData>)));
  EDIF_FORM(ConnectivityViewHeader, "(connectivityViewHeader",
           arg(ref<ConnectivityUnits>) +
           many(arg(ref<DerivedFrom> | ref<Documentation> | ref<NameInformation> | ref<PreviousVersion> | ref<Property> | ref<Status>)));
  EDIF_FORM(ConstantValues, "(constantValues",
           many(arg(ref<BooleanConstant> | ref<IntegerConstant> | ref<StringConstant>)));
  EDIF_FORM(Contract, "(contract",
           arg(stringTokenExpr));
  EDIF_FORM(ContractDisplay, "(contractDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(Copyright, "(copyright",
           arg(ref<Year>) +
           many(arg(stringTokenExpr)));
  EDIF_FORM(CopyrightDisplay, "(copyrightDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(CornerType, "(cornerType",
           arg(ref<Truncate> | ref<Round> | ref<Extend>));
  EDIF_FORM(Coulomb, "(coulomb",
           arg(unitExponentExpr));
  EDIF_FORM(Criticality, "(criticality",
           arg(integerValueExpr));
  EDIF_FORM(CriticalityDisplay, "(criticalityDisplay",
           many(arg(ref<Display>)));
  EDIF_FORM(CurrentMap, "(currentMap",
           arg(currentValueExpr));
  EDIF_FORM(Curve, "(curve",
           many(arg(ref<Arc> | pointValueExpr)));
  EDIF_FORM(DataOrigin, "(dataOrigin",
           arg(stringTokenExpr) +
           option(arg(ref<Version>)));
  EDIF_FORM(Date, "(date",
           arg(yearNumberExpr) +
           arg(monthNumberExpr) +
           arg(dayNumberExpr));
  EDIF_FORM(DcFanInLoad, "(dcFanInLoad",
           arg(numberValueExpr));
  EDIF_FORM(DcFanInLoadDisplay, "(dcFanInLoadDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(DcFanOutLoad, "(dcFanOutLoad",
           arg(numberValueExpr));
  EDIF_FORM(DcFanOutLoadDisplay, "(dcFanOutLoadDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(DcMaxFanIn, "(dcMaxFanIn",
           arg(numberValueExpr));
  EDIF_FORM(DcMaxFanInDisplay, "(dcMaxFanInDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(DcMaxFanOut, "(dcMaxFanOut",
           arg(numberValueExpr));
  EDIF_FORM(DcMaxFanOutDisplay, "(dcMaxFanOutDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(DecimalToString, "(decimalToString",
           arg(integerExpressionExpr) +
           option(arg(ref<MinimumStringLength>)));
  EDIF_FORM(DefaultClusterConfiguration, "(defaultClusterConfiguration",
           arg(clusterConfigurationNameRefExpr));
  EDIF_FORM(DefaultConnection, "(defaultConnection",
           arg(ref<GlobalPortRef>));
  EDIF_FORM(Degree, "(degree",
           arg(unitExponentExpr));
  EDIF_FORM(Delay, "(delay",
           arg(timeDelayExpr));
  EDIF_FORM(DerivedFrom, "(derivedFrom",
           arg(ref<ViewRef>) +
           option(arg(ref<Reason>)));
  EDIF_FORM(Design, "(design",
           arg(designNameDefExpr) +
           arg(ref<CellRef>) +
           arg(ref<DesignHeader>) +
           many(arg(ref<Comment> | ref<DesignHierarchy> | ref<UserData>)));
  EDIF_FORM(Designator, "(designator",
           arg(stringValueExpr));
  EDIF_FORM(DesignatorDisplay, "(designatorDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(DesignHeader, "(designHeader",
           arg(ref<DesignUnits>) +
           many(arg(ref<Documentation> | ref<NameInformation> | ref<Property> | ref<Status>)));
  EDIF_FORM(DesignHierarchy, "(designHierarchy",
           arg(designHierarchyNameDefExpr) +
           arg(ref<ClusterRef>) +
           arg(ref<ClusterConfigurationRef>) +
           arg(ref<DesignHierarchyHeader>) +
           option(arg(ref<OccurrenceHierarchyAnnotate>)));
  EDIF_FORM(DesignHierarchyHeader, "(designHierarchyHeader",
           many(arg(ref<BooleanParameterAssign> | ref<IntegerParameterAssign> | ref<NameInformation> | ref<NumberParameterAssign> | ref<Property> | ref<StringParameterAssign>)));
  EDIF_FORM(DesignHierarchyNameCaseSensitive, "(designHierarchyNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(DesignNameCaseSensitive, "(designNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(DesignUnits, "(designUnits",
           many(arg(ref<SetCapacitance> | ref<SetTime>)));
  EDIF_FORM(Diagram, "(diagram",
           arg(diagramNameDefExpr) +
           many(arg(ref<Annotate> | ref<Comment> | ref<Figure> | ref<UserData>)));
  EDIF_FORM(DirectionalPortAttributeOverride, "(directionalPortAttributeOverride",
           arg(ref<InputPortAttributes> | ref<OutputPortAttributes> | ref<BidirectionalPortAttributes>));
  EDIF_FORM(Display, "(display",
           many(arg(figureGroupNameRefExpr | ref<FigureGroupOverride>)) +
           arg(ref<Transform>));
  EDIF_FORM(DisplayAttributes, "(displayAttributes",
           many(arg(ref<BorderPattern> | ref<BorderPatternVisible> | ref<BorderWidth> | ref<Color> | ref<FillPattern> | ref<FillPatternVisible> | ref<FontRef> | ref<HorizontalJustification> | ref<TextHeight> | ref<VerticalJustification> | ref<Visible>)));
  EDIF_FORM(DisplayName, "(displayName",
           arg(stringTokenExpr));
  EDIF_FORM(DisplayNameOverride, "(displayNameOverride",
           arg(stringTokenExpr) +
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(Documentation, "(documentation",
           arg(documentationNameDefExpr) +
           arg(ref<DocumentationHeader>) +
           many(arg(ref<Section>)));
  EDIF_FORM(DocumentationHeader, "(documentationHeader",
           arg(ref<DocumentationUnits>) +
           many(arg(ref<BackgroundColor> | ref<NameInformation> | ref<Status>)));
  EDIF_FORM(DocumentationNameCaseSensitive, "(documentationNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(DocumentationUnits, "(documentationUnits",
           many(arg(ref<SetAngle> | ref<SetDistance>)));
  EDIF_FORM(Dominates, "(dominates",
           many(arg(logicNameRefExpr)));
  EDIF_FORM(Dot, "(dot",
           arg(pointValueExpr));
  EDIF_FORM(DrawingDescription, "(drawingDescription",
           arg(stringTokenExpr));
  EDIF_FORM(DrawingDescriptionDisplay, "(drawingDescriptionDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(DrawingIdentification, "(drawingIdentification",
           arg(stringTokenExpr));
  EDIF_FORM(DrawingIdentificationDisplay, "(drawingIdentificationDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(DrawingSize, "(drawingSize",
           arg(stringTokenExpr));
  EDIF_FORM(DrawingSizeDisplay, "(drawingSizeDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(Duration, "(duration",
           arg(timeValueExpr));
  EDIF_FORM(E, "(e",
           arg(mantissaExpr) +
           arg(exponentExpr));
  EDIF_FORM(Edif, "(edif",
           arg(edifNameDefExpr) +
           arg(ref<EdifVersion>) +
           arg(ref<EdifHeader>) +
           many(arg(ref<Library> | ref<Design> | ref<Comment> | ref<External> | ref<UserData>)));
  EDIF_FORM(EdifHeader, "(edifHeader",
           arg(ref<EdifLevel>) +
           arg(ref<KeywordMap>) +
           arg(ref<UnitDefinitions>) +
           arg(ref<FontDefinitions>) +
           arg(ref<PhysicalDefaults>) +
           many(arg(ref<CharacterEncoding> | ref<ConstantValues> | ref<Documentation> | ref<GlobalPortDefinitions> | ref<NameCaseSensitivity> | ref<NameInformation> | ref<PhysicalScaling> | ref<Property> | ref<Status>)));
  EDIF_FORM(EdifLevel, "(edifLevel",
           arg(edifLevelValueExpr));
  EDIF_FORM(EdifVersion, "(edifVersion",
           arg(markExpr) +
           arg(issueExpr) +
           arg(subIssueExpr));
  EDIF_FORM(EndType, "(endType",
           arg(ref<Truncate> | ref<Round> | ref<Extend>));
  EDIF_FORM(EngineeringDate, "(engineeringDate",
           arg(ref<Date>));
  EDIF_FORM(EngineeringDateDisplay, "(engineeringDateDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(Event, "(event",
           arg(ref<PortRef> | ref<PortList> | ref<PortSet> | ref<InterconnectRef> | ref<InterconnectSet>) +
           many(arg(ref<Becomes> | ref<Transition>)));
  EDIF_FORM0(Extend, "(extend");
  EDIF_FORM(External, "(external",
           arg(libraryNameDefExpr) +
           arg(ref<LibraryHeader>) +
           many(arg(ref<Cell> | ref<Comment> | ref<GeometryMacro> | ref<PageBorderTemplate> | ref<PageTitleBlockTemplate> | ref<SchematicFigureMacro> | ref<SchematicForFrameBorderTemplate> | ref<SchematicGlobalPortTemplate> | ref<SchematicIfFrameBorderTemplate> | ref<SchematicInterconnectTerminatorTemplate> | ref<SchematicJunctionTemplate> | ref<SchematicMasterPortTemplate> | ref<SchematicOffPageConnectorTemplate> | ref<SchematicOnPageConnectorTemplate> | ref<SchematicOtherwiseFrameBorderTemplate> | ref<SchematicRipperTemplate> | ref<SchematicSymbolBorderTemplate> | ref<SchematicSymbolPortTemplate> | ref<UserData>)));
  EDIF_FORM(Fahrenheit, "(fahrenheit",
           arg(unitExponentExpr));
  EDIF_FORM0(Efalse, "(false");
  EDIF_FORM(Farad, "(farad",
           arg(unitExponentExpr));
  EDIF_FORM(Figure, "(figure",
           arg(figureGroupNameRefExpr | ref<FigureGroupOverride>) +
           many(arg(ref<Circle> | ref<Comment> | ref<ComplexGeometry> | ref<Dot> | ref<OpenShape> | ref<Path> | ref<Polygon> | ref<Rectangle> | ref<Shape> | ref<UserData>)));
  EDIF_FORM(FigureGroup, "(figureGroup",
           arg(figureGroupNameDefExpr) +
           many(arg(ref<Comment> | ref<CornerType> | ref<DisplayAttributes> | ref<EndType> | ref<NameInformation> | ref<PathWidth> | ref<Property> | ref<UserData>)));
  EDIF_FORM(FigureGroupNameCaseSensitive, "(figureGroupNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(FigureGroupOverride, "(figureGroupOverride",
           arg(figureGroupNameRefExpr) +
           many(arg(ref<Comment> | ref<CornerType> | ref<DisplayAttributes> | ref<EndType> | ref<PathWidth> | ref<PropertyOverride>)));
  EDIF_FORM(FillPattern, "(fillPattern",
           arg(ref<PixelPattern>));
  EDIF_FORM(FillPatternVisible, "(fillPatternVisible",
           arg(booleanValueExpr));
  EDIF_FORM0(Fixed, "(fixed");
  EDIF_FORM(Font, "(font",
           arg(fontNameDefExpr) +
           arg(ref<Typeface>) +
           arg(ref<FontProportions>) +
           many(arg(ref<BoldStyle> | ref<ItalicStyle> | ref<Property> | ref<ProportionalFont> | ref<UserData>)));
  EDIF_FORM(FontCapitalHeight, "(fontCapitalHeight",
           arg(lengthValueExpr));
  EDIF_FORM(FontDefinitions, "(fontDefinitions",
           option(arg(ref<Fonts>)));
  EDIF_FORM(FontDescent, "(fontDescent",
           arg(lengthValueExpr));
  EDIF_FORM(FontHeight, "(fontHeight",
           arg(lengthValueExpr));
  EDIF_FORM(FontProportions, "(fontProportions",
           arg(ref<FontHeight>) +
           arg(ref<FontDescent>) +
           arg(ref<FontCapitalHeight>) +
           arg(ref<FontWidth>));
  EDIF_FORM(FontRef, "(fontRef",
           arg(fontNameRefExpr));
  EDIF_FORM(Fonts, "(fonts",
           arg(ref<SetDistance>) +
           many(arg(ref<Font>)));
  EDIF_FORM(FontWidth, "(fontWidth",
           arg(lengthValueExpr));
  EDIF_FORM(ForbiddenEvent, "(forbiddenEvent",
           arg(ref<TimeInterval>) +
           many(arg(ref<Event>)));
  EDIF_FORM(ForFrame, "(forFrame",
           arg(frameNameDefExpr) +
           arg(ref<RepetitionCount>) +
           arg(ref<ForFrameIndex>) +
           arg(ref<LogicalConnectivity>) +
           many(arg(ref<Comment> | ref<Documentation> | ref<NameInformation> | ref<Property> | ref<UserData>)));
  EDIF_FORM(ForFrameAnnotate, "(forFrameAnnotate",
           arg(extendForFrameMemberDefExpr) +
           many(arg(ref<Comment> | ref<ForFrameAnnotate> | ref<IfFrameAnnotate> | ref<InterconnectAnnotate> | ref<LeafOccurrenceAnnotate> | ref<OccurrenceAnnotate> | ref<OtherwiseFrameAnnotate>)));
  EDIF_FORM(ForFrameIndex, "(forFrameIndex",
           arg(indexNameDefExpr) +
           many(arg(ref<IndexStart> | ref<IndexStep> | ref<NameInformation>)));
  EDIF_FORM(ForFrameIndexDisplay, "(forFrameIndexDisplay",
           many(arg(ref<IndexEndDisplay> | ref<IndexNameDisplay> | ref<IndexStartDisplay> | ref<IndexStepDisplay>)));
  EDIF_FORM(ForFrameIndexNameCaseSensitive, "(forFrameIndexNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(ForFrameIndexRef, "(forFrameIndexRef",
           arg(indexNameRefExpr));
  EDIF_FORM(ForFrameMemberRef, "(forFrameMemberRef",
           arg(frameNameRefExpr) +
           arg(ref<IndexValue>));
  EDIF_FORM(ForFrameRef, "(forFrameRef",
           arg(frameNameRefExpr));
  EDIF_FORM(FrameConfiguration, "(frameConfiguration",
           arg(frameNameRefExpr) +
           many(arg(ref<FrameConfiguration> | ref<InstanceConfiguration>)));
  EDIF_FORM(FrameNameCaseSensitive, "(frameNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(FrameRef, "(frameRef",
           arg(frameNameRefExpr));
  EDIF_FORM0(FromBottom, "(fromBottom");
  EDIF_FORM0(FromLeft, "(fromLeft");
  EDIF_FORM0(FromRight, "(fromRight");
  EDIF_FORM0(FromTop, "(fromTop");
  EDIF_FORM0(Generated, "(generated");
  EDIF_FORM(GeometryMacro, "(geometryMacro",
           arg(libraryObjectNameDefExpr) +
           arg(ref<GeometryMacroHeader>) +
           many(arg(ref<Circle> | ref<Comment> | ref<ComplexGeometry> | ref<Dot> | ref<OpenShape> | ref<Path> | ref<Polygon> | ref<Rectangle> | ref<Shape> | ref<UserData>)));
  EDIF_FORM(GeometryMacroHeader, "(geometryMacroHeader",
           arg(ref<GeometryMacroUnits>) +
           many(arg(ref<BackgroundColor> | ref<Documentation> | ref<NameInformation> | ref<OriginalBoundingBox> | ref<Property> | ref<Status>)));
  EDIF_FORM(GeometryMacroRef, "(geometryMacroRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(GeometryMacroUnits, "(geometryMacroUnits",
           option(arg(ref<SetAngle>)));
  EDIF_FORM(GlobalPort, "(globalPort",
           arg(globalPortNameDefExpr) +
           many(arg(ref<Comment> | ref<NameInformation> | ref<Property> | ref<SchematicGlobalPortAttributes> | ref<UserData>)));
  EDIF_FORM(GlobalPortBundle, "(globalPortBundle",
           arg(globalPortNameDefExpr) +
           arg(ref<GlobalPortList>) +
           many(arg(ref<Comment> | ref<NameInformation> | ref<Property> | ref<UserData>)));
  EDIF_FORM(GlobalPortDefinitions, "(globalPortDefinitions",
           many(arg(ref<GlobalPort> | ref<GlobalPortBundle>)));
  EDIF_FORM(GlobalPortList, "(globalPortList",
           many(arg(ref<GlobalPortRef>)));
  EDIF_FORM(GlobalPortNameCaseSensitive, "(globalPortNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(GlobalPortNameDisplay, "(globalPortNameDisplay",
           many(arg(ref<Display> | ref<DisplayNameOverride>)));
  EDIF_FORM(GlobalPortPropertyDisplay, "(globalPortPropertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<Display> | ref<PropertyNameDisplay>)));
  EDIF_FORM(GlobalPortRef, "(globalPortRef",
           arg(globalPortNameRefExpr));
  EDIF_FORM(GlobalPortScope, "(globalPortScope",
           arg(globalPortNameRefExpr));
  EDIF_FORM(Henry, "(henry",
           arg(unitExponentExpr));
  EDIF_FORM(Hertz, "(hertz",
           arg(unitExponentExpr));
  EDIF_FORM(HorizontalJustification, "(horizontalJustification",
           arg(ref<LeftJustify> | ref<CenterJustify> | ref<RightJustify>));
  EDIF_FORM(Hotspot, "(hotspot",
           arg(pointValueExpr) +
           many(arg(ref<HotspotConnectDirection> | ref<HotspotNameDisplay> | ref<NameInformation> | ref<SchematicWireAffinity>)));
  EDIF_FORM(HotspotConnectDirection, "(hotspotConnectDirection",
           many(arg(ref<FromBottom> | ref<FromLeft> | ref<FromRight> | ref<FromTop>)));
  EDIF_FORM(HotspotGrid, "(hotspotGrid",
           arg(lengthValueExpr));
  EDIF_FORM(HotspotNameCaseSensitive, "(hotspotNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(HotspotNameDisplay, "(hotspotNameDisplay",
           many(arg(ref<Display> | ref<DisplayNameOverride>)));
  EDIF_FORM(IeeeSection, "(ieeeSection",
           many(arg(integerTokenExpr)));
  EDIF_FORM(IeeeStandard, "(ieeeStandard",
           arg(ieeeDesignationExpr) +
           arg(ref<Year>) +
           many(arg(ref<Comment> | ref<IeeeSection>)));
  EDIF_FORM(IfFrame, "(ifFrame",
           arg(frameNameDefExpr) +
           arg(ref<Condition>) +
           arg(ref<LogicalConnectivity>) +
           many(arg(ref<Comment> | ref<NameInformation> | ref<Documentation> | ref<Property> | ref<UserData>)));
  EDIF_FORM(IfFrameAnnotate, "(ifFrameAnnotate",
           arg(extendFrameDefExpr) +
           many(arg(ref<Comment> | ref<ForFrameAnnotate> | ref<IfFrameAnnotate> | ref<InterconnectAnnotate> | ref<LeafOccurrenceAnnotate> | ref<OccurrenceAnnotate> | ref<OtherwiseFrameAnnotate> | ref<PropertyOverride>)));
  EDIF_FORM(IfFrameRef, "(ifFrameRef",
           arg(frameNameRefExpr));
  EDIF_FORM(IfFrameSet, "(ifFrameSet",
           many(arg(ref<IfFrameRef>)));
  EDIF_FORM0(Ignore, "(ignore");
  EDIF_FORM(ImplementationNameCaseSensitive, "(implementationNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(ImplementationNameDisplay, "(implementationNameDisplay",
           many(arg(ref<Display> | ref<DisplayNameOverride>)));
  EDIF_FORM(IndexEndDisplay, "(indexEndDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(IndexNameDisplay, "(indexNameDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(IndexStart, "(indexStart",
           arg(integerExpressionExpr));
  EDIF_FORM(IndexStartDisplay, "(indexStartDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(IndexStep, "(indexStep",
           arg(integerExpressionExpr));
  EDIF_FORM(IndexStepDisplay, "(indexStepDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(IndexValue, "(indexValue",
           arg(integerTokenExpr));
  EDIF_FORM0(Input, "(input");
  EDIF_FORM(InputPort, "(inputPort",
           option(arg(ref<InputPortAttributes>)));
  EDIF_FORM(InputPortAttributes, "(inputPortAttributes",
           many(arg(ref<DcFanOutLoad> | ref<DcMaxFanIn>)));
  EDIF_FORM(Instance, "(instance",
           arg(instanceNameDefExpr) +
           arg(ref<ClusterRef>) +
           many(arg(ref<BooleanParameterAssign> | ref<CellPropertyOverride> | ref<ClusterPropertyOverride> | ref<Comment> | ref<Designator> | ref<InstanceNameGenerator> | ref<InstancePortAttributes> | ref<InstanceWidth> | ref<IntegerParameterAssign> | ref<NameInformation> | ref<NumberParameterAssign> | ref<Property> | ref<StringParameterAssign> | ref<Timing> | ref<UserData>)));
  EDIF_FORM(InstanceConfiguration, "(instanceConfiguration",
           arg(instanceNameRefExpr) +
           arg(ref<ClusterConfigurationRef>));
  EDIF_FORM0(InstanceIndexValue, "(instanceIndexValue");
  EDIF_FORM(InstanceMemberRef, "(instanceMemberRef",
           arg(instanceNameRefExpr) +
           arg(ref<IndexValue>));
  EDIF_FORM(InstanceNameCaseSensitive, "(instanceNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(InstanceNameDisplay, "(instanceNameDisplay",
           many(arg(ref<Display> | ref<DisplayNameOverride>)));
  EDIF_FORM(InstanceNameGenerator, "(instanceNameGenerator",
           arg(stringExpressionExpr));
  EDIF_FORM(InstanceNameGeneratorDisplay, "(instanceNameGeneratorDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(InstancePortAttributeDisplay, "(instancePortAttributeDisplay",
           arg(symbolPortImplementationNameRefExpr) +
           arg(ref<PortRef>) +
           many(arg(ref<PortPropertyDisplayOverride> | ref<PortAttributeDisplay>)));
  EDIF_FORM(InstancePortAttributes, "(instancePortAttributes",
           arg(extendPortDefExpr) +
           many(arg(ref<AcLoad> | ref<Comment> | ref<ConnectedSignalIndexGenerator> | ref<Designator> | ref<DirectionalPortAttributeOverride> | ref<PortDelay> | ref<PortDelayOverride> | ref<PortLoadDelay> | ref<PortLoadDelayOverride> | ref<PortPropertyOverride> | ref<Property> | ref<Unused>)));
  EDIF_FORM(InstancePropertyDisplay, "(instancePropertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<Display>)) +
           option(arg(ref<PropertyNameDisplay>) +
                  many(arg(ref<Display>))));
  EDIF_FORM(InstancePropertyOverride, "(instancePropertyOverride",
           arg(propertyNameRefExpr) +
           arg(typedValueExpr | ref<Untyped>) +
           many(arg(ref<Comment> | ref<Fixed> | ref<PropertyOverride>)));
  EDIF_FORM(InstanceRef, "(instanceRef",
           arg(instanceNameRefExpr));
  EDIF_FORM(InstanceWidth, "(instanceWidth",
           arg(integerExpressionExpr));
  EDIF_FORM(InstanceWidthDisplay, "(instanceWidthDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(Integer, "(integer",
           arg(integerExpressionExpr));
  EDIF_FORM(IntegerConstant, "(integerConstant",
           arg(constantNameDefExpr) +
           arg(integerTokenExpr));
  EDIF_FORM(IntegerConstantRef, "(integerConstantRef",
           arg(constantNameRefExpr));
  EDIF_FORM(IntegerEqual, "(integerEqual",
           arg(firstIntegerExpressionExpr) +
           arg(secondIntegerExpressionExpr));
  EDIF_FORM(IntegerParameter, "(integerParameter",
           arg(parameterNameDefExpr) +
           many(arg(ref<Integer> | ref<NameInformation>)));
  EDIF_FORM(IntegerParameterAssign, "(integerParameterAssign",
           arg(parameterNameRefExpr) +
           arg(integerExpressionExpr));
  EDIF_FORM(IntegerParameterRef, "(integerParameterRef",
           arg(parameterNameRefExpr));
  EDIF_FORM(IntegerProduct, "(integerProduct",
           many(arg(integerExpressionExpr)));
  EDIF_FORM(IntegerQuotient, "(integerQuotient",
           arg(dividendExpr) +
           arg(divisorExpr));
  EDIF_FORM(IntegerRemainder, "(integerRemainder",
           arg(dividendExpr) +
           arg(divisorExpr));
  EDIF_FORM(IntegerSubtract, "(integerSubtract",
           arg(minuendExpr) +
           arg(subtrahendExpr));
  EDIF_FORM(IntegerSum, "(integerSum",
           many(arg(integerExpressionExpr)));
  EDIF_FORM(InterconnectAnnotate, "(interconnectAnnotate",
           arg(extendInterconnectDefExpr) +
           many(arg(ref<Comment> | ref<InterconnectAnnotate> | ref<Criticality> | ref<InterconnectDelay> | ref<Property> | ref<PropertyOverride>)));
  EDIF_FORM(InterconnectAttachedText, "(interconnectAttachedText",
           arg(attachmentPointExpr) +
           many(arg(ref<Annotate> | ref<ConnectivityTagGeneratorDisplay> | ref<CriticalityDisplay> | ref<InterconnectDelayDisplay> | ref<InterconnectNameDisplay> | ref<InterconnectPropertyDisplay>)));
  EDIF_FORM(InterconnectDelay, "(interconnectDelay",
           arg(interconnectDelayNameDefExpr) +
           arg(derivationExpr) +
           arg(ref<Delay>) +
           many(arg(ref<Becomes> | ref<Transition>)));
  EDIF_FORM(InterconnectDelayDisplay, "(interconnectDelayDisplay",
           arg(interconnectDelayNameRefExpr) +
           many(arg(ref<Display>)));
  EDIF_FORM(InterconnectHeader, "(interconnectHeader",
           many(arg(ref<Criticality> | ref<Documentation> | ref<InterconnectDelay> | ref<NameInformation> | ref<Property>)));
  EDIF_FORM(InterconnectNameCaseSensitive, "(interconnectNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(InterconnectNameDisplay, "(interconnectNameDisplay",
           many(arg(ref<Display> | ref<DisplayNameOverride>)));
  EDIF_FORM(InterconnectPropertyDisplay, "(interconnectPropertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<Display>)) +
           option(arg(ref<PropertyNameDisplay>) +
                  many(arg(ref<Display>))));
  EDIF_FORM(InterconnectRef, "(interconnectRef",
           arg(interconnectNameRefExpr) +
           option(arg(ref<PageRef>)));
  EDIF_FORM(InterconnectSet, "(interconnectSet",
           many(arg(ref<InterconnectRef>)));
  EDIF_FORM(Einterface, "(interface",
           arg(ref<InterfaceUnits>) +
           many(arg(ref<Designator> | ref<BooleanParameter> | ref<IntegerParameter> | ref<InterfaceJoined> | ref<MustJoin> | ref<NumberParameter> | ref<Permutable> | ref<Port> | ref<PortBundle> | ref<StringParameter> | ref<Timing> | ref<WeakJoined>)));
  EDIF_FORM(InterfaceJoined, "(interfaceJoined",
           many(arg(ref<PortRef>)));
  EDIF_FORM(InterfaceUnits, "(interfaceUnits",
           many(arg(ref<SetCapacitance> | ref<SetTime>)));
  EDIF_FORM(Iso8859, "(iso8859",
           arg(iso8859PartExpr));
  EDIF_FORM0(Isolated, "(isolated");
  EDIF_FORM0(ItalicStyle, "(italicStyle");
  EDIF_FORM0(Jisx0201, "(jisx0201");
  EDIF_FORM0(Jisx0208, "(jisx0208");
  EDIF_FORM0(JoinedAsSignal, "(joinedAsSignal");
  EDIF_FORM(Joule, "(joule",
           arg(unitExponentExpr));
  EDIF_FORM0(K0KeywordLevel, "(k0KeywordLevel");
  EDIF_FORM(K1KeywordAlias, "(k1KeywordAlias",
           arg(k1KeywordNameDefExpr) +
           arg(k1KeywordNameRefExpr));
  EDIF_FORM(K1KeywordLevel, "(k1KeywordLevel",
           many(arg(ref<K1KeywordAlias>)));
  EDIF_FORM(K2Actual, "(k2Actual",
           arg(k2FormalNameRefExpr));
  EDIF_FORM(K2Build, "(k2Build",
           arg(k1KeywordNameRefExpr) +
           many(arg(ref<Comment> | ref<K2Actual> | ref<K2Build> | ref<K2Literal>)));
  EDIF_FORM(K2Formal, "(k2Formal",
           arg(k2FormalNameDefExpr) +
           arg(ref<K2Optional> | ref<K2Required>));
  EDIF_FORM(K2Generate, "(k2Generate",
           many(arg(ref<Comment> | ref<K2Actual> | ref<K2Build> | ref<K2Literal>)));
  EDIF_FORM(K2KeywordDefine, "(k2KeywordDefine",
           arg(k1KeywordNameDefExpr) +
           arg(ref<K2KeywordParameters>) +
           arg(ref<K2Generate>));
  EDIF_FORM(K2KeywordLevel, "(k2KeywordLevel",
           many(arg(ref<K1KeywordAlias> | ref<K2KeywordDefine>)));
  EDIF_FORM(K2KeywordParameters, "(k2KeywordParameters",
           many(arg(ref<K2Formal>)));
  EDIF_FORM(K2Literal, "(k2Literal",
           many(arg(ref<IDENTIFIER> | integerTokenExpr | stringTokenExpr)));
  EDIF_FORM(K2Optional, "(k2Optional",
           arg(ref<K2Literal> | ref<K2Actual> | ref<K2Build>));
  EDIF_FORM0(K2Required, "(k2Required");
  EDIF_FORM(K3Build, "(k3Build",
           arg(k1KeywordNameRefExpr) +
           many(arg(ref<Comment> | ref<K2Actual> | ref<K2Literal> | ref<K3Build> | ref<K3ForEach>)));
  EDIF_FORM0(K3Collector, "(k3Collector");
  EDIF_FORM(K3ForEach, "(k3ForEach",
           arg(k2FormalNameRefExpr | ref<K3FormalList>) +
           many(arg(ref<Comment> | ref<K2Actual> | ref<K2Literal> | ref<K3Build> | ref<K3ForEach>)));
  EDIF_FORM(K3Formal, "(k3Formal",
           arg(k2FormalNameDefExpr) +
           arg(ref<K2Optional> | ref<K2Required> | ref<K3Collector>));
  EDIF_FORM(K3FormalList, "(k3FormalList",
           many(arg(k2FormalNameRefExpr)));
  EDIF_FORM(K3Generate, "(k3Generate",
           many(arg(ref<Comment> | ref<K2Actual> | ref<K2Build> | ref<K2Literal> | ref<K3ForEach>)));
  EDIF_FORM(K3KeywordDefine, "(k3KeywordDefine",
           arg(k1KeywordNameDefExpr) +
           arg(ref<K3KeywordParameters>) +
           arg(ref<K3Generate>));
  EDIF_FORM(K3KeywordLevel, "(k3KeywordLevel",
           many(arg(ref<K1KeywordAlias> | ref<K3KeywordDefine>)));
  EDIF_FORM(K3KeywordParameters, "(k3KeywordParameters",
           many(arg(ref<K3Formal>)));
  EDIF_FORM(Kelvin, "(kelvin",
           arg(unitExponentExpr));
  EDIF_FORM(KeywordMap, "(keywordMap",
           arg(ref<K0KeywordLevel> | ref<K1KeywordLevel> | ref<K2KeywordLevel> | ref<K3KeywordLevel>) +
           many(arg(ref<Comment>)));
  EDIF_FORM(Kilogram, "(kilogram",
           arg(unitExponentExpr));
  EDIF_FORM0(Leaf, "(leaf");
  EDIF_FORM(LeafOccurrenceAnnotate, "(leafOccurrenceAnnotate",
           arg(extendInstanceDefExpr | extendInstanceMemberDefExpr) +
           many(arg(ref<CellPropertyOverride> | ref<ClusterPropertyOverride> | ref<Comment> | ref<Designator> | ref<InstancePropertyOverride> | ref<PortAnnotate> | ref<Property>)));
  EDIF_FORM0(LeftJustify, "(leftJustify");
  EDIF_FORM(LessThan, "(lessThan",
           many(arg(integerExpressionExpr)));
  EDIF_FORM(LessThanOrEqual, "(lessThanOrEqual",
           many(arg(integerExpressionExpr)));
  EDIF_FORM(Library, "(library",
           arg(libraryNameDefExpr) +
           arg(ref<LibraryHeader>) +
           many(arg(ref<Cell> | ref<SchematicInterconnectTerminatorTemplate> | ref<SchematicJunctionTemplate> | ref<SchematicGlobalPortTemplate> | ref<SchematicMasterPortTemplate> | ref<SchematicOffPageConnectorTemplate> | ref<SchematicOnPageConnectorTemplate> | ref<SchematicRipperTemplate> | ref<SchematicSymbolBorderTemplate> | ref<SchematicSymbolPortTemplate> | ref<PageBorderTemplate> | ref<PageTitleBlockTemplate> | ref<Comment> | ref<GeometryMacro> | ref<SchematicFigureMacro> | ref<SchematicForFrameBorderTemplate> | ref<SchematicIfFrameBorderTemplate> | ref<SchematicOtherwiseFrameBorderTemplate> | ref<UserData>)));
  EDIF_FORM(LibraryHeader, "(libraryHeader",
           arg(ref<EdifLevel>) +
           arg(ref<NameCaseSensitivity>) +
           arg(ref<Technology>) +
           many(arg(ref<BackgroundColor> | ref<Documentation> | ref<NameInformation> | ref<Property> | ref<Status>)));
  EDIF_FORM(LibraryNameCaseSensitive, "(libraryNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(LibraryObjectNameCaseSensitive, "(libraryObjectNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(LibraryRef, "(libraryRef",
           arg(libraryNameRefExpr));
  EDIF_FORM(LoadDelay, "(loadDelay",
           arg(acLoadFactorTimeExpr) +
           arg(acLoadFactorCapacitanceExpr));
  EDIF_FORM(LocalPortGroup, "(localPortGroup",
           arg(localPortGroupNameDefExpr) +
           arg(ref<PortList>) +
           many(arg(ref<Comment> | ref<NameInformation> | ref<Property> | ref<UserData>)));
  EDIF_FORM(LocalPortGroupNameCaseSensitive, "(localPortGroupNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(LocalPortGroupRef, "(localPortGroupRef",
           arg(localPortGroupNameRefExpr));
  EDIF_FORM(LogicalConnectivity, "(logicalConnectivity",
           many(arg(ref<Comment> | ref<ForFrame> | ref<IfFrame> | ref<Instance> | ref<OtherwiseFrame> | ref<Signal> | ref<SignalGroup> | ref<UserData>)));
  EDIF_FORM(LogicDefinitions, "(logicDefinitions",
           arg(ref<SetVoltage>) +
           arg(ref<SetCurrent>) +
           many(arg(ref<Comment> | ref<LogicValue>)));
  EDIF_FORM(LogicList, "(logicList",
           many(arg(ref<Ignore> | logicNameRefExpr | ref<LogicOneOf>)));
  EDIF_FORM(LogicMapInput, "(logicMapInput",
           many(arg(ref<LogicRef>)));
  EDIF_FORM(LogicMapOutput, "(logicMapOutput",
           many(arg(ref<LogicRef>)));
  EDIF_FORM(LogicModelUnits, "(logicModelUnits",
           many(arg(ref<SetCapacitance> | ref<SetTime>)));
  EDIF_FORM(LogicModelView, "(logicModelView",
           arg(viewNameDefExpr) +
           many(arg(ref<Comment> | ref<NameInformation> | ref<UserData>)));
  EDIF_FORM(LogicOneOf, "(logicOneOf",
           many(arg(ref<LogicList> | logicNameRefExpr)));
  EDIF_FORM(LogicRef, "(logicRef",
           arg(logicNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(LogicValue, "(logicValue",
           arg(logicNameDefExpr) +
           many(arg(ref<BooleanMap> | ref<Comment> | ref<Compound> | ref<CurrentMap> | ref<Dominates> | ref<Isolated> | ref<LogicMapInput> | ref<LogicMapOutput> | ref<NameInformation> | ref<Property> | ref<Resolves> | ref<Strong> | ref<VoltageMap> | ref<Weak>)));
  EDIF_FORM0(LsbToMsb, "(lsbToMsb");
  EDIF_FORM(MaskLayoutUnits, "(maskLayoutUnits",
           many(arg(ref<SetAngle> | ref<SetCapacitance> | ref<SetDistance> | ref<SetTime>)));
  EDIF_FORM(MaskLayoutView, "(maskLayoutView",
           arg(viewNameDefExpr) +
           many(arg(ref<Comment> | ref<NameInformation> | ref<UserData>)));
  EDIF_FORM0(Measured, "(measured");
  EDIF_FORM(Meter, "(meter",
           arg(unitExponentExpr));
  EDIF_FORM0(MiddleJustify, "(middleJustify");
  EDIF_FORM0(MinimalWidth, "(minimalWidth");
  EDIF_FORM(MinimumStringLength, "(minimumStringLength",
           arg(substringLengthExpr));
  EDIF_FORM(MiNoMax, "(miNoMax",
           arg(miNoMaxValueExpr));
  EDIF_FORM0(MixedDirection, "(mixedDirection");
  EDIF_FORM(Mnm, "(mnm",
           arg(numberValueExpr | ref<Undefined> | ref<Unconstrained>) +
           arg(numberValueExpr | ref<Undefined> | ref<Unconstrained>) +
           arg(numberValueExpr | ref<Undefined> | ref<Unconstrained>));
  EDIF_FORM(Mole, "(mole",
           arg(unitExponentExpr));
  EDIF_FORM0(MsbToLsb, "(msbToLsb");
  EDIF_FORM(MustJoin, "(mustJoin",
           many(arg(ref<InterfaceJoined> | ref<PortRef> | ref<WeakJoined>)));
  EDIF_FORM(NameAlias, "(nameAlias",
           arg(originalNameExpr) +
           many(arg(ref<DisplayName> | ref<Generated> | ref<NameStructure>)));
  EDIF_FORM(NameCaseSensitivity, "(nameCaseSensitivity",
           many(arg(ref<ClusterConfigurationNameCaseSensitive> | ref<ClusterNameCaseSensitive> | ref<DesignHierarchyNameCaseSensitive> | ref<DesignNameCaseSensitive> | ref<DocumentationNameCaseSensitive> | ref<FigureGroupNameCaseSensitive> | ref<ForFrameIndexNameCaseSensitive> | ref<FrameNameCaseSensitive> | ref<GlobalPortNameCaseSensitive> | ref<HotspotNameCaseSensitive> | ref<ImplementationNameCaseSensitive> | ref<InstanceNameCaseSensitive> | ref<InterconnectNameCaseSensitive> | ref<LibraryNameCaseSensitive> | ref<LibraryObjectNameCaseSensitive> | ref<LocalPortGroupNameCaseSensitive> | ref<PageNameCaseSensitive> | ref<ParameterNameCaseSensitive> | ref<PortNameCaseSensitive> | ref<PropertyNameCaseSensitive> | ref<SignalGroupNameCaseSensitive> | ref<SignalNameCaseSensitive> | ref<ViewGroupNameCaseSensitive> | ref<ViewNameCaseSensitive>)));
  EDIF_FORM(NameDimension, "(nameDimension",
           arg(ref<NameDimensionStructure>) +
           option(arg(ref<BitOrder>)));
  EDIF_FORM(NameDimensionStructure, "(nameDimensionStructure",
           many(arg(ref<ComplementedName> | ref<ComplexName> | integerValueExpr | ref<Sequence> | simpleNameExpr)));
  EDIF_FORM(NameInformation, "(nameInformation",
           arg(ref<PrimaryName>) +
           many(arg(ref<NameAlias>)));
  EDIF_FORM(NamePartSeparator, "(namePartSeparator",
           arg(stringTokenExpr));
  EDIF_FORM(NameStructure, "(nameStructure",
           arg(simpleNameExpr | ref<ComplexName> | ref<ComplementedName>));
  EDIF_FORM0(NarrowPort, "(narrowPort");
  EDIF_FORM0(NarrowWire, "(narrowWire");
  EDIF_FORM0(NoHotspotGrid, "(noHotspotGrid");
  EDIF_FORM(NominalHotspotGrid, "(nominalHotspotGrid",
           arg(lengthValueExpr));
  EDIF_FORM(NonPermutable, "(nonPermutable",
           many(arg(ref<Permutable> | ref<PortRef>)));
  EDIF_FORM(Not, "(not",
           arg(booleanExpressionExpr));
  EDIF_FORM0(NotInherited, "(notInherited");
  EDIF_FORM(Number, "(number",
           arg(numberExpressionExpr));
  EDIF_FORM(NumberParameter, "(numberParameter",
           arg(parameterNameDefExpr) +
           many(arg(ref<NameInformation> | ref<Number>)));
  EDIF_FORM(NumberParameterAssign, "(numberParameterAssign",
           arg(parameterNameRefExpr) +
           arg(numberExpressionExpr));
  EDIF_FORM(NumberParameterRef, "(numberParameterRef",
           arg(parameterNameRefExpr));
  EDIF_FORM(NumberPoint, "(numberPoint",
           arg(xNumberValueExpr) +
           arg(yNumberValueExpr));
  EDIF_FORM(OccurrenceAnnotate, "(occurrenceAnnotate",
           arg(extendInstanceDefExpr | extendInstanceMemberDefExpr) +
           many(arg(ref<CellPropertyOverride> | ref<ClusterPropertyOverride> | ref<Comment> | ref<Designator> | ref<ForFrameAnnotate> | ref<IfFrameAnnotate> | ref<InstancePropertyOverride> | ref<InterconnectAnnotate> | ref<LeafOccurrenceAnnotate> | ref<OccurrenceAnnotate> | ref<OtherwiseFrameAnnotate> | ref<PageAnnotate> | ref<PortAnnotate> | ref<Property> | ref<SignalAnnotate> | ref<SignalGroupAnnotate> | ref<Timing> | ref<ViewPropertyOverride>)));
  EDIF_FORM(OccurrenceHierarchyAnnotate, "(occurrenceHierarchyAnnotate",
           many(arg(ref<CellPropertyOverride> | ref<ClusterPropertyOverride> | ref<Comment> | ref<ForFrameAnnotate> | ref<IfFrameAnnotate> | ref<InterconnectAnnotate> | ref<LeafOccurrenceAnnotate> | ref<OccurrenceAnnotate> | ref<OtherwiseFrameAnnotate> | ref<PageAnnotate> | ref<PortAnnotate> | ref<SignalAnnotate> | ref<SignalGroupAnnotate> | ref<Timing> | ref<ViewPropertyOverride>)));
  EDIF_FORM(OffsetEvent, "(offsetEvent",
           arg(ref<Event>) +
           arg(numberValueExpr));
  EDIF_FORM(Ohm, "(ohm",
           arg(unitExponentExpr));
  EDIF_FORM(OpenShape, "(openShape",
           arg(ref<Curve>));
  EDIF_FORM(Or, "(or",
           many(arg(booleanExpressionExpr)));
  EDIF_FORM(Origin, "(origin",
           arg(pointValueExpr));
  EDIF_FORM(OriginalBoundingBox, "(originalBoundingBox",
           arg(ref<Rectangle>));
  EDIF_FORM(OriginalDrawingDate, "(originalDrawingDate",
           arg(ref<Date>));
  EDIF_FORM(OriginalDrawingDateDisplay, "(originalDrawingDateDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(OtherwiseFrame, "(otherwiseFrame",
           arg(frameNameDefExpr) +
           arg(ref<IfFrameSet>) +
           arg(ref<LogicalConnectivity>) +
           many(arg(ref<Comment> | ref<Documentation> | ref<NameInformation> | ref<Property> | ref<UserData>)));
  EDIF_FORM(OtherwiseFrameAnnotate, "(otherwiseFrameAnnotate",
           arg(extendFrameDefExpr) +
           many(arg(ref<Comment> | ref<ForFrameAnnotate> | ref<IfFrameAnnotate> | ref<InterconnectAnnotate> | ref<LeafOccurrenceAnnotate> | ref<OccurrenceAnnotate> | ref<OtherwiseFrameAnnotate> | ref<PropertyOverride>)));
  EDIF_FORM(OtherwiseFrameRef, "(otherwiseFrameRef",
           arg(frameNameRefExpr));
  EDIF_FORM0(Output, "(output");
  EDIF_FORM(OutputPort, "(outputPort",
           option(arg(ref<OutputPortAttributes>)));
  EDIF_FORM(OutputPortAttributes, "(outputPortAttributes",
           many(arg(ref<DcFanInLoad> | ref<DcMaxFanOut>)));
  EDIF_FORM(Owner, "(owner",
           arg(stringValueExpr));
  EDIF_FORM(Page, "(page",
           arg(pageNameDefExpr) +
           arg(ref<PageHeader>) +
           many(arg(ref<CellPropertyDisplay> | ref<ClusterPropertyDisplay> | ref<Comment> | ref<LocalPortGroup> | ref<PageCommentGraphics> | ref<PageTitleBlock> | ref<PropertyDisplay> | ref<SchematicBus> | ref<SchematicForFrameImplementation> | ref<SchematicGlobalPortImplementation> | ref<SchematicIfFrameImplementation> | ref<SchematicInstanceImplementation> | ref<SchematicMasterPortImplementation> | ref<SchematicNet> | ref<SchematicOffPageConnectorImplementation> | ref<SchematicOnPageConnectorImplementation> | ref<SchematicOtherwiseFrameImplementation> | ref<SchematicRipperImplementation> | ref<UserData> | ref<ViewPropertyDisplay>)));
  EDIF_FORM(PageAnnotate, "(pageAnnotate",
           arg(extendPageDefExpr) +
           many(arg(ref<InterconnectAnnotate>)));
  EDIF_FORM(PageBorder, "(pageBorder",
           arg(ref<PageBorderTemplateRef>) +
           arg(ref<Transform>) +
           many(arg(ref<PropertyDisplayOverride> | ref<PropertyOverride>)));
  EDIF_FORM(PageBorderTemplate, "(pageBorderTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeader>) +
           arg(ref<UsableArea>) +
           many(arg(ref<Annotate> | ref<CommentGraphics> | ref<Figure> | ref<PropertyDisplay> | ref<SchematicComplexFigure>)));
  EDIF_FORM(PageBorderTemplateRef, "(pageBorderTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(PageCommentGraphics, "(pageCommentGraphics",
           many(arg(ref<Annotate> | ref<Comment> | ref<Figure> | ref<SchematicComplexFigure> | ref<UserData>)));
  EDIF_FORM(PageHeader, "(pageHeader",
           many(arg(ref<BackgroundColor> | ref<Documentation> | ref<NameInformation> | ref<OriginalBoundingBox> | ref<PageBorder> | ref<PageSize> | ref<Property> | ref<Status>)));
  EDIF_FORM(PageIdentification, "(pageIdentification",
           arg(stringTokenExpr));
  EDIF_FORM(PageIdentificationDisplay, "(pageIdentificationDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(PageNameCaseSensitive, "(pageNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(PagePropertyDisplay, "(pagePropertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<Display> | ref<PropertyNameDisplay>)));
  EDIF_FORM(PageRef, "(pageRef",
           arg(pageNameRefExpr));
  EDIF_FORM(PageSize, "(pageSize",
           arg(ref<Rectangle>));
  EDIF_FORM(PageTitle, "(pageTitle",
           arg(stringTokenExpr));
  EDIF_FORM(PageTitleBlock, "(pageTitleBlock",
           arg(implementationNameDefExpr) +
           arg(ref<PageTitleBlockTemplateRef>) +
           arg(ref<Transform>) +
           many(arg(ref<NameInformation> | ref<PagePropertyDisplay> | ref<PageTitleBlockAttributeDisplay> | ref<PageTitleBlockAttributes> | ref<Property> | ref<PropertyDisplay> | ref<PropertyDisplayOverride> | ref<PropertyOverride>)));
  EDIF_FORM(PageTitleBlockAttributeDisplay, "(pageTitleBlockAttributeDisplay",
           many(arg(ref<ApprovedDateDisplay> | ref<CheckDateDisplay> | ref<CompanyNameDisplay> | ref<ContractDisplay> | ref<CopyrightDisplay> | ref<DrawingDescriptionDisplay> | ref<DrawingIdentificationDisplay> | ref<DrawingSizeDisplay> | ref<EngineeringDateDisplay> | ref<OriginalDrawingDateDisplay> | ref<PageIdentificationDisplay> | ref<PageTitleDisplay> | ref<RevisionDisplay> | ref<TotalPagesDisplay>)));
  EDIF_FORM(PageTitleBlockAttributes, "(pageTitleBlockAttributes",
           many(arg(ref<ApprovedDate> | ref<CheckDate> | ref<CompanyName> | ref<Contract> | ref<DrawingDescription> | ref<DrawingIdentification> | ref<DrawingSize> | ref<EngineeringDate> | ref<OriginalDrawingDate> | ref<PageIdentification> | ref<PageTitle> | ref<Revision>)));
  EDIF_FORM(PageTitleBlockTemplate, "(pageTitleBlockTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeader>) +
           many(arg(ref<Annotate> | ref<CommentGraphics> | ref<Figure> | ref<PageTitleBlockAttributeDisplay> | ref<PageTitleBlockAttributes> | ref<PropertyDisplay> | ref<SchematicComplexFigure>)));
  EDIF_FORM(PageTitleBlockTemplateRef, "(pageTitleBlockTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(PageTitleDisplay, "(pageTitleDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(ParameterDisplay, "(parameterDisplay",
           arg(parameterNameRefExpr) +
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>) +
           option(arg(ref<ParameterNameDisplay>)));
  EDIF_FORM(ParameterNameCaseSensitive, "(parameterNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(ParameterNameDisplay, "(parameterNameDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(Path, "(path",
           arg(ref<PointList>));
  EDIF_FORM(PathDelay, "(pathDelay",
           arg(ref<Delay>) +
           many(arg(ref<Event>)));
  EDIF_FORM(PathWidth, "(pathWidth",
           arg(lengthValueExpr | ref<MinimalWidth>));
  EDIF_FORM(PcbLayoutUnits, "(pcbLayoutUnits",
           many(arg(ref<SetAngle> | ref<SetCapacitance> | ref<SetDistance> | ref<SetTime>)));
  EDIF_FORM(PcbLayoutView, "(pcbLayoutView",
           arg(viewNameDefExpr) +
           many(arg(ref<Comment> | ref<NameInformation> | ref<UserData>)));
  EDIF_FORM(Permutable, "(permutable",
           many(arg(ref<NonPermutable> | ref<Permutable> | ref<PortRef>)));
  EDIF_FORM(PhysicalDefaults, "(physicalDefaults",
           option(arg(ref<SchematicRequiredDefaults>)));
  EDIF_FORM(PhysicalScaling, "(physicalScaling",
           many(arg(ref<Comment> | ref<ConnectivityUnits> | ref<DocumentationUnits> | ref<GeometryMacroUnits> | ref<InterfaceUnits> | ref<LogicModelUnits> | ref<MaskLayoutUnits> | ref<PcbLayoutUnits> | ref<SchematicUnits> | ref<SymbolicLayoutUnits>)));
  EDIF_FORM(PixelPattern, "(pixelPattern",
           arg(rowSizeExpr) +
           many(arg(ref<PixelRow>)));
  EDIF_FORM(PixelRow, "(pixelRow",
           many(arg(booleanTokenExpr)));
  EDIF_FORM(Point, "(point",
           arg(pointValueExpr));
  EDIF_FORM(PointList, "(pointList",
           many(arg(pointValueExpr)));
  EDIF_FORM(Polygon, "(polygon",
           arg(ref<PointList>));
  EDIF_FORM(Port, "(port",
           option(arg(portNameDefExpr)) +
           option(arg(portDirectionExpr)) +
           many(arg(ref<AcLoad> | ref<Comment> | ref<DefaultConnection> | ref<Designator> | ref<NameInformation> | ref<PortDelay> | ref<PortLoadDelay> | ref<PortNameGenerator> | ref<PortWidth> | ref<Property> | ref<SchematicPortAttributes> | ref<Unused> | ref<UserData>)));
  EDIF_FORM(PortAnnotate, "(portAnnotate",
           arg(extendPortDefExpr | extendPortMemberDefExpr) +
           many(arg(ref<AcLoad> | ref<Comment> | ref<Designator> | ref<DirectionalPortAttributeOverride> | ref<PortDelay> | ref<PortDelayOverride> | ref<PortLoadDelay> | ref<PortLoadDelayOverride> | ref<PortPropertyOverride> | ref<Property>)));
  EDIF_FORM(PortAttributeDisplay, "(portAttributeDisplay",
           many(arg(ref<AcLoadDisplay> | ref<ConnectedSignalIndexGeneratorDisplay> | ref<DcFanInLoadDisplay> | ref<DcFanOutLoadDisplay> | ref<DcMaxFanInDisplay> | ref<DcMaxFanOutDisplay> | ref<DesignatorDisplay> | ref<PortDelayDisplay> | ref<PortLoadDelayDisplay> | ref<PortNameDisplay> | ref<PortNameGeneratorDisplay> | ref<PortPropertyDisplay>)));
  EDIF_FORM(PortBundle, "(portBundle",
           arg(portNameDefExpr) +
           arg(ref<PortList>) +
           many(arg(ref<Comment> | ref<NameInformation> | ref<Property> | ref<UserData> | ref<Designator>)));
  EDIF_FORM(PortDelay, "(portDelay",
           arg(portDelayNameDefExpr) +
           arg(derivationExpr) +
           arg(ref<Delay>) +
           many(arg(ref<Becomes> | ref<Transition>)));
  EDIF_FORM(PortDelayDisplay, "(portDelayDisplay",
           arg(portDelayNameRefExpr) +
           many(arg(ref<Display>)));
  EDIF_FORM(PortDelayOverride, "(portDelayOverride",
           arg(portDelayNameRefExpr) +
           arg(derivationExpr) +
           arg(ref<Delay>) +
           many(arg(ref<Becomes> | ref<Transition>)));
  EDIF_FORM0(PortIndexValue, "(portIndexValue");
  EDIF_FORM(PortInstanceRef, "(portInstanceRef",
           arg(portNameRefExpr | ref<PortMemberRef>) +
           arg(ref<InstanceRef> | ref<InstanceMemberRef>));
  EDIF_FORM(PortJoined, "(portJoined",
           many(arg(ref<GlobalPortRef> | ref<LocalPortGroupRef> | ref<PortInstanceRef> | ref<PortRef>)));
  EDIF_FORM(PortList, "(portList",
           many(arg(ref<PortRef>)));
  EDIF_FORM(PortLoadDelay, "(portLoadDelay",
           arg(portLoadDelayNameDefExpr) +
           arg(derivationExpr) +
           arg(ref<LoadDelay>) +
           many(arg(ref<Becomes> | ref<Transition>)));
  EDIF_FORM(PortLoadDelayDisplay, "(portLoadDelayDisplay",
           arg(portLoadDelayNameRefExpr) +
           many(arg(ref<Display>)));
  EDIF_FORM(PortLoadDelayOverride, "(portLoadDelayOverride",
           arg(portLoadDelayNameRefExpr) +
           arg(derivationExpr) +
           arg(ref<LoadDelay>) +
           many(arg(ref<Becomes> | ref<Transition>)));
  EDIF_FORM(PortMemberRef, "(portMemberRef",
           arg(portNameRefExpr) +
           arg(ref<IndexValue>));
  EDIF_FORM(PortNameCaseSensitive, "(portNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(PortNameDisplay, "(portNameDisplay",
           many(arg(ref<Display> | ref<DisplayNameOverride>)));
  EDIF_FORM(PortNameGenerator, "(portNameGenerator",
           arg(stringExpressionExpr));
  EDIF_FORM(PortNameGeneratorDisplay, "(portNameGeneratorDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(PortPropertyDisplay, "(portPropertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<Display> | ref<PropertyNameDisplay>)));
  EDIF_FORM(PortPropertyDisplayOverride, "(portPropertyDisplayOverride",
           arg(propertyNameRefExpr) +
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>) +
           option(arg(ref<PropertyNameDisplay>)));
  EDIF_FORM(PortPropertyOverride, "(portPropertyOverride",
           arg(propertyNameRefExpr) +
           arg(typedValueExpr | ref<Untyped>) +
           many(arg(ref<Comment> | ref<Fixed> | ref<PropertyOverride>)));
  EDIF_FORM(PortRef, "(portRef",
           arg(portNameRefExpr));
  EDIF_FORM(PortSet, "(portSet",
           many(arg(ref<PortRef>)));
  EDIF_FORM(PortWidth, "(portWidth",
           arg(integerExpressionExpr));
  EDIF_FORM(PreviousVersion, "(previousVersion",
           arg(ref<ViewRef>) +
           option(arg(ref<Reason>)));
  EDIF_FORM(PrimaryName, "(primaryName",
           arg(originalNameExpr) +
           many(arg(ref<DisplayName> | ref<Generated> | ref<NameStructure>)));
  EDIF_FORM(Program, "(program",
           arg(stringValueExpr) +
           option(arg(ref<Version>)));
  EDIF_FORM(Property, "(property",
           arg(propertyNameDefExpr) +
           arg(typedValueExpr | ref<Untyped>) +
           many(arg(ref<Comment> | ref<NameInformation> | ref<Owner> | ref<Property> | ref<PropertyInheritanceControl> | ref<UnitRef>)));
  EDIF_FORM(PropertyDisplay, "(propertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<Display> | ref<PropertyNameDisplay>)));
  EDIF_FORM(PropertyDisplayOverride, "(propertyDisplayOverride",
           arg(propertyNameRefExpr) +
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>) +
           option(arg(ref<PropertyNameDisplay>)));
  EDIF_FORM(PropertyInheritanceControl, "(propertyInheritanceControl",
           arg(ref<Fixed> | ref<NotInherited>));
  EDIF_FORM(PropertyNameCaseSensitive, "(propertyNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(PropertyNameDisplay, "(propertyNameDisplay",
           many(arg(ref<Display> | ref<DisplayNameOverride>)));
  EDIF_FORM(PropertyOverride, "(propertyOverride",
           arg(propertyNameRefExpr) +
           arg(typedValueExpr | ref<Untyped>) +
           many(arg(ref<Comment> | ref<Fixed> | ref<PropertyOverride>)));
  EDIF_FORM0(ProportionalFont, "(proportionalFont");
  EDIF_FORM(Pt, "(pt",
           arg(xCoordinateExpr) +
           arg(yCoordinateExpr));
  EDIF_FORM(Radian, "(radian",
           arg(unitExponentExpr));
  EDIF_FORM(Reason, "(reason",
           arg(stringTokenExpr));
  EDIF_FORM(Rectangle, "(rectangle",
           arg(pt1Expr) +
           arg(pt2Expr));
  EDIF_FORM0(RemoveDisplay, "(removeDisplay");
  EDIF_FORM(RepetitionCount, "(repetitionCount",
           arg(integerExpressionExpr));
  EDIF_FORM(RepetitionCountDisplay, "(repetitionCountDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(ReplaceDisplay, "(replaceDisplay",
           many(arg(ref<Display>)));
  EDIF_FORM0(Required, "(required");
  EDIF_FORM(Resolves, "(resolves",
           many(arg(logicNameRefExpr)));
  EDIF_FORM(Revision, "(revision",
           arg(stringTokenExpr));
  EDIF_FORM(RevisionDisplay, "(revisionDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM0(RightJustify, "(rightJustify");
  EDIF_FORM(RipperHotspot, "(ripperHotspot",
           arg(hotspotNameDefExpr) +
           arg(ref<Hotspot>));
  EDIF_FORM(RipperHotspotRef, "(ripperHotspotRef",
           arg(hotspotNameRefExpr) +
           arg(ref<SchematicRipperImplementationRef>));
  EDIF_FORM(Rotation, "(rotation",
           arg(angleValueExpr));
  EDIF_FORM0(Round, "(round");
  EDIF_FORM(ScaleX, "(scaleX",
           arg(numeratorExpr) +
           arg(denominatorExpr));
  EDIF_FORM(ScaleY, "(scaleY",
           arg(numeratorExpr) +
           arg(denominatorExpr));
  EDIF_FORM(SchematicBus, "(schematicBus",
           arg(interconnectNameDefExpr) +
           arg(ref<SignalGroupRef>) +
           arg(ref<SchematicInterconnectHeader>) +
           arg(ref<SchematicBusJoined>) +
           many(arg(ref<Comment> | ref<SchematicBusDetails> | ref<SchematicBusSlice> | ref<SchematicInterconnectAttributeDisplay> | ref<UserData>)));
  EDIF_FORM(SchematicBusDetails, "(schematicBusDetails",
           arg(ref<SchematicBusGraphics> | ref<SchematicSubBusSet>));
  EDIF_FORM(SchematicBusGraphics, "(schematicBusGraphics",
           many(arg(ref<Comment> | ref<Figure> | ref<SchematicComplexFigure> | ref<UserData>)));
  EDIF_FORM(SchematicBusJoined, "(schematicBusJoined",
           many(arg(ref<PortJoined> | ref<RipperHotspotRef> | ref<SchematicGlobalPortImplementationRef> | ref<SchematicInterconnectTerminatorImplementationRef> | ref<SchematicJunctionImplementationRef> | ref<SchematicMasterPortImplementationRef> | ref<SchematicOffPageConnectorImplementationRef> | ref<SchematicOnPageConnectorImplementationRef> | ref<SchematicSymbolPortImplementationRef>)));
  EDIF_FORM(SchematicBusSlice, "(schematicBusSlice",
           arg(interconnectNameDefExpr) +
           arg(ref<SignalGroupRef>) +
           arg(ref<SchematicInterconnectHeader>) +
           arg(ref<SchematicBusJoined>) +
           many(arg(ref<Comment> | ref<SchematicBusDetails> | ref<SchematicBusSlice> | ref<SchematicInterconnectAttributeDisplay> | ref<UserData>)));
  EDIF_FORM(SchematicComplexFigure, "(schematicComplexFigure",
           arg(ref<SchematicFigureMacroRef>) +
           arg(ref<Transform>) +
           many(arg(ref<PropertyDisplayOverride> | ref<PropertyOverride>)));
  EDIF_FORM(SchematicFigureMacro, "(schematicFigureMacro",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeader>) +
           many(arg(ref<Annotate> | ref<Comment> | ref<CommentGraphics> | ref<Figure> | ref<PropertyDisplay> | ref<SchematicComplexFigure> | ref<UserData>)));
  EDIF_FORM(SchematicFigureMacroRef, "(schematicFigureMacroRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(SchematicForFrameBorder, "(schematicForFrameBorder",
           arg(ref<SchematicForFrameBorderTemplateRef>) +
           arg(ref<Transform>) +
           many(arg(ref<ForFrameIndexDisplay> | ref<PropertyDisplayOverride> | ref<PropertyOverride> | ref<RepetitionCountDisplay>)));
  EDIF_FORM(SchematicForFrameBorderTemplate, "(schematicForFrameBorderTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeader>) +
           arg(ref<UsableArea>) +
           many(arg(ref<Annotate> | ref<CommentGraphics> | ref<Figure> | ref<ForFrameIndexDisplay> | ref<PropertyDisplay> | ref<RepetitionCountDisplay> | ref<SchematicComplexFigure>)));
  EDIF_FORM(SchematicForFrameBorderTemplateRef, "(schematicForFrameBorderTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(SchematicForFrameImplementation, "(schematicForFrameImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<ForFrameRef>) +
           arg(ref<SchematicForFrameImplementationHeader>) +
           arg(ref<SchematicFrameImplementationDetails>));
  EDIF_FORM(SchematicForFrameImplementationHeader, "(schematicForFrameImplementationHeader",
           option(arg(ref<SchematicForFrameBorder>)));
  EDIF_FORM(SchematicFrameImplementationDetails, "(schematicFrameImplementationDetails",
           many(arg(ref<CellPropertyDisplay> | ref<ClusterPropertyDisplay> | ref<Comment> | ref<CommentGraphics> | ref<PropertyDisplay> | ref<SchematicBus> | ref<SchematicForFrameImplementation> | ref<SchematicGlobalPortImplementation> | ref<SchematicIfFrameImplementation> | ref<SchematicInstanceImplementation> | ref<SchematicMasterPortImplementation> | ref<SchematicNet> | ref<SchematicOffPageConnectorImplementation> | ref<SchematicOnPageConnectorImplementation> | ref<SchematicOtherwiseFrameImplementation> | ref<SchematicRipperImplementation> | ref<UserData> | ref<ViewPropertyDisplay>)));
  EDIF_FORM(SchematicGlobalPortAttributes, "(schematicGlobalPortAttributes",
           many(arg(ref<IeeeStandard> | ref<SchematicPortAcPower> | ref<SchematicPortAnalog> | ref<SchematicPortChassisGround> | ref<SchematicPortClock> | ref<SchematicPortDcPower> | ref<SchematicPortEarthGround> | ref<SchematicPortGround> | ref<SchematicPortNonLogical> | ref<SchematicPortOpenCollector> | ref<SchematicPortOpenEmitter> | ref<SchematicPortPower> | ref<SchematicPortThreeState>)));
  EDIF_FORM(SchematicGlobalPortImplementation, "(schematicGlobalPortImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<SchematicGlobalPortTemplateRef>) +
           arg(ref<GlobalPortRef>) +
           arg(ref<Transform>) +
           many(arg(ref<GlobalPortNameDisplay> | ref<GlobalPortPropertyDisplay> | ref<ImplementationNameDisplay> | ref<NameInformation> | ref<PropertyDisplayOverride> | ref<PropertyOverride>)));
  EDIF_FORM(SchematicGlobalPortImplementationRef, "(schematicGlobalPortImplementationRef",
           arg(implementationNameRefExpr));
  EDIF_FORM(SchematicGlobalPortTemplate, "(schematicGlobalPortTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeader>) +
           option(arg(ref<Hotspot>)) +
           many(arg(ref<Annotate> | ref<CommentGraphics> | ref<Figure> | ref<GlobalPortNameDisplay> | ref<ImplementationNameDisplay> | ref<PropertyDisplay> | ref<SchematicComplexFigure> | ref<SchematicGlobalPortAttributes>)) +
           option(arg(ref<Hotspot>)));
  EDIF_FORM(SchematicGlobalPortTemplateRef, "(schematicGlobalPortTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(SchematicIfFrameBorder, "(schematicIfFrameBorder",
           arg(ref<SchematicIfFrameBorderTemplateRef>) +
           arg(ref<Transform>) +
           many(arg(ref<ConditionDisplay> | ref<PropertyDisplayOverride> | ref<PropertyOverride>)));
  EDIF_FORM(SchematicIfFrameBorderTemplate, "(schematicIfFrameBorderTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeader>) +
           arg(ref<UsableArea>) +
           many(arg(ref<Annotate> | ref<CommentGraphics> | ref<ConditionDisplay> | ref<Figure> | ref<PropertyDisplay> | ref<SchematicComplexFigure>)));
  EDIF_FORM(SchematicIfFrameBorderTemplateRef, "(schematicIfFrameBorderTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(SchematicIfFrameImplementation, "(schematicIfFrameImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<IfFrameRef>) +
           arg(ref<SchematicIfFrameImplementationHeader>) +
           arg(ref<SchematicFrameImplementationDetails>));
  EDIF_FORM(SchematicIfFrameImplementationHeader, "(schematicIfFrameImplementationHeader",
           option(arg(ref<SchematicIfFrameBorder>)));
  EDIF_FORM(SchematicImplementation, "(schematicImplementation",
           many(arg(ref<Page> | ref<TotalPages>)));
  EDIF_FORM(SchematicInstanceImplementation, "(schematicInstanceImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<InstanceRef>) +
           arg(ref<SchematicSymbolRef>) +
           arg(ref<Transform>) +
           many(arg(ref<CellNameDisplay> | ref<CellPropertyDisplayOverride> | ref<ClusterPropertyDisplayOverride> | ref<DesignatorDisplay> | ref<ImplementationNameDisplay> | ref<InstanceNameDisplay> | ref<InstanceNameGeneratorDisplay> | ref<InstancePortAttributeDisplay> | ref<InstancePropertyDisplay> | ref<InstanceWidthDisplay> | ref<NameInformation> | ref<PageCommentGraphics> | ref<ParameterDisplay> | ref<PropertyDisplayOverride> | ref<PropertyOverride> | ref<TimingDisplay> | ref<ViewNameDisplay>)));
  EDIF_FORM(SchematicInstanceImplementationRef, "(schematicInstanceImplementationRef",
           arg(implementationNameRefExpr));
  EDIF_FORM(SchematicInterconnectAttributeDisplay, "(schematicInterconnectAttributeDisplay",
           many(arg(ref<ConnectivityTagGeneratorDisplay> | ref<CriticalityDisplay> | ref<InterconnectAttachedText> | ref<InterconnectDelayDisplay> | ref<InterconnectNameDisplay> | ref<InterconnectPropertyDisplay>)));
  EDIF_FORM(SchematicInterconnectHeader, "(schematicInterconnectHeader",
           many(arg(ref<Criticality> | ref<Documentation> | ref<InterconnectDelay> | ref<NameInformation> | ref<Property> | ref<SchematicInterconnectTerminatorImplementation> | ref<SchematicJunctionImplementation> | ref<SchematicWireStyle>)));
  EDIF_FORM(SchematicInterconnectTerminatorImplementation, "(schematicInterconnectTerminatorImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<SchematicInterconnectTerminatorTemplateRef>) +
           arg(ref<Transform>) +
           many(arg(ref<ImplementationNameDisplay> | ref<NameInformation> | ref<PropertyDisplayOverride> | ref<PropertyOverride>)));
  EDIF_FORM(SchematicInterconnectTerminatorImplementationRef, "(schematicInterconnectTerminatorImplementationRef",
           arg(implementationNameRefExpr));
  EDIF_FORM(SchematicInterconnectTerminatorTemplate, "(schematicInterconnectTerminatorTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeader>) +
           arg(ref<Hotspot>) +
           many(arg(ref<CommentGraphics> | ref<Figure> | ref<ImplementationNameDisplay> | ref<PropertyDisplay> | ref<SchematicComplexFigure>)));
  EDIF_FORM(SchematicInterconnectTerminatorTemplateRef, "(schematicInterconnectTerminatorTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(SchematicJunctionImplementation, "(schematicJunctionImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<SchematicJunctionTemplateRef>) +
           arg(ref<Transform>) +
           many(arg(ref<ImplementationNameDisplay> | ref<NameInformation> | ref<PropertyDisplayOverride> | ref<PropertyOverride>)));
  EDIF_FORM(SchematicJunctionImplementationRef, "(schematicJunctionImplementationRef",
           arg(implementationNameRefExpr));
  EDIF_FORM(SchematicJunctionTemplate, "(schematicJunctionTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeader>) +
           arg(ref<Hotspot>) +
           many(arg(ref<CommentGraphics> | ref<Figure> | ref<ImplementationNameDisplay> | ref<PropertyDisplay> | ref<SchematicComplexFigure>)));
  EDIF_FORM(SchematicJunctionTemplateRef, "(schematicJunctionTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(SchematicMasterPortImplementation, "(schematicMasterPortImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<SchematicMasterPortTemplateRef>) +
           arg(ref<PortRef> | ref<LocalPortGroupRef>) +
           arg(ref<Transform>) +
           many(arg(ref<ImplementationNameDisplay> | ref<NameInformation> | ref<PortAttributeDisplay> | ref<PropertyDisplayOverride> | ref<PropertyOverride>)));
  EDIF_FORM(SchematicMasterPortImplementationRef, "(schematicMasterPortImplementationRef",
           arg(implementationNameRefExpr));
  EDIF_FORM(SchematicMasterPortTemplate, "(schematicMasterPortTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeader>) +
           arg(ref<Hotspot>) +
           arg(portDirectionIndicatorExpr) +
           many(arg(ref<Annotate> | ref<CommentGraphics> | ref<Figure> | ref<ImplementationNameDisplay> | ref<PortAttributeDisplay> | ref<PropertyDisplay> | ref<SchematicComplexFigure> | ref<SchematicPortStyle>)));
  EDIF_FORM(SchematicMasterPortTemplateRef, "(schematicMasterPortTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(SchematicMetric, "(schematicMetric",
           arg(ref<SetDistance>) +
           arg(ref<HotspotGrid> | ref<NoHotspotGrid>) +
           option(arg(ref<NominalHotspotGrid>)));
  EDIF_FORM(SchematicNet, "(schematicNet",
           arg(interconnectNameDefExpr) +
           arg(ref<SignalRef>) +
           arg(ref<SchematicInterconnectHeader>) +
           arg(ref<SchematicNetJoined>) +
           many(arg(ref<Comment> | ref<SchematicInterconnectAttributeDisplay> | ref<SchematicNetDetails> | ref<UserData>)));
  EDIF_FORM(SchematicNetDetails, "(schematicNetDetails",
           arg(ref<SchematicNetGraphics> | ref<SchematicSubNetSet>));
  EDIF_FORM(SchematicNetGraphics, "(schematicNetGraphics",
           many(arg(ref<Comment> | ref<Figure> | ref<SchematicComplexFigure> | ref<UserData>)));
  static constexpr auto schematicNetJoinedRefExpr = ref<RipperHotspotRef> | ref<SchematicGlobalPortImplementationRef> | ref<SchematicInterconnectTerminatorImplementationRef> | ref<SchematicJunctionImplementationRef> | ref<SchematicMasterPortImplementationRef> | ref<SchematicOffPageConnectorImplementationRef> | ref<SchematicOnPageConnectorImplementationRef> | ref<SchematicSymbolPortImplementationRef>;
  static constexpr auto schematicNetJoinedPortExpr = ref<PortJoined> | ref<JoinedAsSignal>;
  EDIF_FORM(SchematicNetJoined, "(schematicNetJoined",
           option((some(arg(schematicNetJoinedRefExpr)) |
                   (arg(schematicNetJoinedPortExpr) +
                    many(arg(schematicNetJoinedRefExpr)))) +
                  option(arg(schematicNetJoinedPortExpr) +
                         many(arg(schematicNetJoinedRefExpr)))));
  EDIF_FORM(SchematicOffPageConnectorImplementation, "(schematicOffPageConnectorImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<SchematicOffPageConnectorTemplateRef>) +
           arg(ref<Transform>) +
           many(arg(ref<AssociatedInterconnectNameDisplay> | ref<ImplementationNameDisplay> | ref<NameInformation> | ref<Property> | ref<PropertyDisplay> | ref<PropertyDisplayOverride> | ref<PropertyOverride>)));
  EDIF_FORM(SchematicOffPageConnectorImplementationRef, "(schematicOffPageConnectorImplementationRef",
           arg(implementationNameRefExpr));
  EDIF_FORM(SchematicOffPageConnectorTemplate, "(schematicOffPageConnectorTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeader>) +
           arg(ref<Hotspot>) +
           many(arg(ref<Annotate> | ref<AssociatedInterconnectNameDisplay> | ref<CommentGraphics> | ref<Figure> | ref<ImplementationNameDisplay> | ref<PropertyDisplay> | ref<SchematicComplexFigure>)));
  EDIF_FORM(SchematicOffPageConnectorTemplateRef, "(schematicOffPageConnectorTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(SchematicOnPageConnectorImplementation, "(schematicOnPageConnectorImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<SchematicOnPageConnectorTemplateRef>) +
           arg(ref<Transform>) +
           many(arg(ref<AssociatedInterconnectNameDisplay> | ref<ImplementationNameDisplay> | ref<NameInformation> | ref<Property> | ref<PropertyDisplay> | ref<PropertyDisplayOverride> | ref<PropertyOverride>)));
  EDIF_FORM(SchematicOnPageConnectorImplementationRef, "(schematicOnPageConnectorImplementationRef",
           arg(implementationNameRefExpr));
  EDIF_FORM(SchematicOnPageConnectorTemplate, "(schematicOnPageConnectorTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeader>) +
           arg(ref<Hotspot>) +
           many(arg(ref<Annotate> | ref<AssociatedInterconnectNameDisplay> | ref<CommentGraphics> | ref<Figure> | ref<ImplementationNameDisplay> | ref<PropertyDisplay> | ref<SchematicComplexFigure>)));
  EDIF_FORM(SchematicOnPageConnectorTemplateRef, "(schematicOnPageConnectorTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(SchematicOtherwiseFrameBorder, "(schematicOtherwiseFrameBorder",
           arg(ref<SchematicOtherwiseFrameBorderTemplateRef>) +
           arg(ref<Transform>) +
           many(arg(ref<PropertyDisplayOverride> | ref<PropertyOverride>)));
  EDIF_FORM(SchematicOtherwiseFrameBorderTemplate, "(schematicOtherwiseFrameBorderTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeader>) +
           arg(ref<UsableArea>) +
           many(arg(ref<Annotate> | ref<CommentGraphics> | ref<Figure> | ref<PropertyDisplay> | ref<SchematicComplexFigure>)));
  EDIF_FORM(SchematicOtherwiseFrameBorderTemplateRef, "(schematicOtherwiseFrameBorderTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(SchematicOtherwiseFrameImplementation, "(schematicOtherwiseFrameImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<OtherwiseFrameRef>) +
           arg(ref<SchematicOtherwiseFrameImplementationHeader>) +
           arg(ref<SchematicFrameImplementationDetails>));
  EDIF_FORM(SchematicOtherwiseFrameImplementationHeader, "(schematicOtherwiseFrameImplementationHeader",
           option(arg(ref<SchematicOtherwiseFrameBorder>)));
  EDIF_FORM(SchematicPortAcPower, "(schematicPortAcPower",
           many(arg(ref<SchematicPortAcPowerRecommendedFrequency> | ref<SchematicPortAcPowerRecommendedVoltageRms>)));
  EDIF_FORM(SchematicPortAcPowerRecommendedFrequency, "(schematicPortAcPowerRecommendedFrequency",
           arg(frequencyValueExpr));
  EDIF_FORM(SchematicPortAcPowerRecommendedVoltageRms, "(schematicPortAcPowerRecommendedVoltageRms",
           arg(voltageValueExpr));
  EDIF_FORM0(SchematicPortAnalog, "(schematicPortAnalog");
  EDIF_FORM(SchematicPortAttributes, "(schematicPortAttributes",
           many(arg(ref<IeeeStandard> | ref<SchematicPortAcPower> | ref<SchematicPortAnalog> | ref<SchematicPortChassisGround> | ref<SchematicPortClock> | ref<SchematicPortDcPower> | ref<SchematicPortEarthGround> | ref<SchematicPortGround> | ref<SchematicPortNonLogical> | ref<SchematicPortOpenCollector> | ref<SchematicPortOpenEmitter> | ref<SchematicPortPower> | ref<SchematicPortThreeState>)));
  EDIF_FORM(SchematicPortChassisGround, "(schematicPortChassisGround",
           option(arg(ref<SchematicPortAnalog>)));
  EDIF_FORM(SchematicPortClock, "(schematicPortClock",
           option(arg(ref<IeeeStandard>)));
  EDIF_FORM(SchematicPortDcPower, "(schematicPortDcPower",
           many(arg(ref<SchematicPortAnalog> | ref<SchematicPortDcPowerRecommendedVoltage>)));
  EDIF_FORM(SchematicPortDcPowerRecommendedVoltage, "(schematicPortDcPowerRecommendedVoltage",
           arg(voltageValueExpr));
  EDIF_FORM(SchematicPortEarthGround, "(schematicPortEarthGround",
           option(arg(ref<SchematicPortAnalog>)));
  EDIF_FORM(SchematicPortGround, "(schematicPortGround",
           option(arg(ref<SchematicPortAnalog>)));
  EDIF_FORM0(SchematicPortNonLogical, "(schematicPortNonLogical");
  EDIF_FORM0(SchematicPortOpenCollector, "(schematicPortOpenCollector");
  EDIF_FORM0(SchematicPortOpenEmitter, "(schematicPortOpenEmitter");
  EDIF_FORM0(SchematicPortPower, "(schematicPortPower");
  EDIF_FORM(SchematicPortStyle, "(schematicPortStyle",
           arg(ref<NarrowPort> | ref<WidePort>));
  EDIF_FORM0(SchematicPortThreeState, "(schematicPortThreeState");
  EDIF_FORM(SchematicRequiredDefaults, "(schematicRequiredDefaults",
           arg(ref<SchematicMetric>) +
           arg(ref<FontRef>) +
           arg(ref<TextHeight>));
  EDIF_FORM(SchematicRipperImplementation, "(schematicRipperImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<SchematicRipperTemplateRef>) +
           arg(ref<Transform>) +
           many(arg(ref<ImplementationNameDisplay> | ref<NameInformation> | ref<Property> | ref<PropertyDisplay> | ref<PropertyDisplayOverride> | ref<PropertyOverride>)));
  EDIF_FORM(SchematicRipperImplementationRef, "(schematicRipperImplementationRef",
           arg(implementationNameRefExpr));
  EDIF_FORM(SchematicRipperTemplate, "(schematicRipperTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeader>) +
           many(arg(ref<Annotate> | ref<CommentGraphics> | ref<Figure> | ref<ImplementationNameDisplay> | ref<PropertyDisplay> | ref<RipperHotspot> | ref<SchematicComplexFigure>)));
  EDIF_FORM(SchematicRipperTemplateRef, "(schematicRipperTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(SchematicSubBus, "(schematicSubBus",
           arg(interconnectNameDefExpr) +
           arg(ref<SchematicSubInterconnectHeader>) +
           arg(ref<SchematicBusJoined>) +
           many(arg(ref<Comment> | ref<SchematicBusDetails> | ref<SchematicBusSlice> | ref<SchematicInterconnectAttributeDisplay> | ref<UserData>)));
  EDIF_FORM(SchematicSubBusSet, "(schematicSubBusSet",
           many(arg(ref<SchematicSubBus>)));
  EDIF_FORM(SchematicSubInterconnectHeader, "(schematicSubInterconnectHeader",
           many(arg(ref<Criticality> | ref<Documentation> | ref<InterconnectDelay> | ref<NameInformation> | ref<Property>)));
  EDIF_FORM(SchematicSubNet, "(schematicSubNet",
           arg(interconnectNameDefExpr) +
           arg(ref<SchematicSubInterconnectHeader>) +
           arg(ref<SchematicNetJoined>) +
           many(arg(ref<Comment> | ref<SchematicInterconnectAttributeDisplay> | ref<SchematicNetDetails> | ref<UserData>)));
  EDIF_FORM(SchematicSubNetSet, "(schematicSubNetSet",
           many(arg(ref<SchematicSubNet>)));
  EDIF_FORM(SchematicSymbol, "(schematicSymbol",
           arg(viewNameDefExpr) +
           arg(ref<SchematicSymbolHeader>) +
           many(arg(ref<Annotate> | ref<CellNameDisplay> | ref<CellPropertyDisplay> | ref<ClusterPropertyDisplay> | ref<Comment> | ref<CommentGraphics> | ref<DesignatorDisplay> | ref<Figure> | ref<ImplementationNameDisplay> | ref<InstanceNameDisplay> | ref<InstanceNameGeneratorDisplay> | ref<InstanceWidthDisplay> | ref<ParameterDisplay> | ref<PropertyDisplay> | ref<SchematicComplexFigure> | ref<SchematicSymbolPortImplementation> | ref<UserData> | ref<ViewNameDisplay>)));
  EDIF_FORM(SchematicSymbolBorder, "(schematicSymbolBorder",
           arg(ref<SchematicSymbolBorderTemplateRef>) +
           arg(ref<Transform>) +
           many(arg(ref<PropertyDisplayOverride> | ref<PropertyOverride>)));
  EDIF_FORM(SchematicSymbolBorderTemplate, "(schematicSymbolBorderTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeader>) +
           arg(ref<UsableArea>) +
           many(arg(ref<Annotate> | ref<CommentGraphics> | ref<Figure> | ref<PropertyDisplay> | ref<SchematicComplexFigure>)));
  EDIF_FORM(SchematicSymbolBorderTemplateRef, "(schematicSymbolBorderTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(SchematicSymbolHeader, "(schematicSymbolHeader",
           arg(ref<SchematicUnits>) +
           many(arg(ref<BackgroundColor> | ref<DerivedFrom> | ref<Documentation> | ref<NameInformation> | ref<OriginalBoundingBox> | ref<PageSize> | ref<PreviousVersion> | ref<Property> | ref<SchematicSymbolBorder> | ref<Status>)));
  EDIF_FORM(SchematicSymbolPortImplementation, "(schematicSymbolPortImplementation",
           arg(symbolPortImplementationNameDefExpr) +
           arg(ref<PortRef>) +
           arg(ref<SchematicSymbolPortTemplateRef>) +
           arg(ref<Transform>) +
           many(arg(ref<PortAttributeDisplay> | ref<PropertyDisplayOverride> | ref<PropertyOverride>)));
  EDIF_FORM(SchematicSymbolPortImplementationRef, "(schematicSymbolPortImplementationRef",
           arg(symbolPortImplementationNameRefExpr) +
           arg(ref<SchematicInstanceImplementationRef>));
  EDIF_FORM(SchematicSymbolPortTemplate, "(schematicSymbolPortTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeader>) +
           arg(ref<Hotspot>) +
           arg(portDirectionIndicatorExpr) +
           many(arg(ref<Annotate> | ref<CommentGraphics> | ref<Figure> | ref<ImplementationNameDisplay> | ref<PortAttributeDisplay> | ref<PropertyDisplay> | ref<SchematicComplexFigure> | ref<SchematicPortAttributes> | ref<SchematicPortStyle>)));
  EDIF_FORM(SchematicSymbolPortTemplateRef, "(schematicSymbolPortTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRef>)));
  EDIF_FORM(SchematicSymbolRef, "(schematicSymbolRef",
           arg(viewNameRefExpr));
  EDIF_FORM(SchematicTemplateHeader, "(schematicTemplateHeader",
           arg(ref<SchematicUnits>) +
           many(arg(ref<BackgroundColor> | ref<Documentation> | ref<NameInformation> | ref<OriginalBoundingBox> | ref<Property> | ref<Status>)));
  EDIF_FORM(SchematicUnits, "(schematicUnits",
           many(arg(ref<SchematicMetric> | ref<SetAngle> | ref<SetCapacitance> | ref<SetFrequency> | ref<SetTime> | ref<SetVoltage>)));
  EDIF_FORM(SchematicView, "(schematicView",
           arg(viewNameDefExpr) +
           arg(ref<SchematicViewHeader>) +
           arg(ref<LogicalConnectivity>) +
           arg(ref<SchematicImplementation>) +
           many(arg(ref<Comment> | ref<UserData>)));
  EDIF_FORM(SchematicViewHeader, "(schematicViewHeader",
           arg(ref<SchematicUnits>) +
           many(arg(ref<DerivedFrom> | ref<Documentation> | ref<NameInformation> | ref<PreviousVersion> | ref<Property> | ref<Status>)));
  EDIF_FORM(SchematicWireAffinity, "(schematicWireAffinity",
           arg(ref<NarrowWire> | ref<WideWire> | ref<Unrestricted>));
  EDIF_FORM(SchematicWireStyle, "(schematicWireStyle",
           arg(ref<NarrowWire> | ref<WideWire>));
  EDIF_FORM(Second, "(second",
           arg(unitExponentExpr));
  EDIF_FORM(Section, "(section",
           arg(sectionNameDefExpr) +
           arg(ref<SectionTitle>) +
           many(arg(ref<Diagram> | ref<Section> | stringValueExpr)));
  EDIF_FORM(SectionTitle, "(sectionTitle",
           arg(stringTokenExpr));
  EDIF_FORM(Sequence, "(sequence",
           arg(fromIntegerExpr) +
           arg(toIntegerExpr) +
           option(arg(ref<Step>)));
  EDIF_FORM(SetAngle, "(setAngle",
           arg(ref<UnitRef>));
  EDIF_FORM(SetCapacitance, "(setCapacitance",
           arg(ref<UnitRef>));
  EDIF_FORM(SetCurrent, "(setCurrent",
           arg(ref<UnitRef>));
  EDIF_FORM(SetDistance, "(setDistance",
           arg(ref<UnitRef>));
  EDIF_FORM(SetFrequency, "(setFrequency",
           arg(ref<UnitRef>));
  EDIF_FORM(SetTime, "(setTime",
           arg(ref<UnitRef>));
  EDIF_FORM(SetVoltage, "(setVoltage",
           arg(ref<UnitRef>));
  EDIF_FORM(Shape, "(shape",
           arg(ref<Curve>));
  EDIF_FORM(Siemens, "(siemens",
           arg(unitExponentExpr));
  EDIF_FORM(Signal, "(signal",
           arg(signalNameDefExpr) +
           arg(ref<SignalJoined>) +
           many(arg(ref<ConnectivityTagGenerator> | ref<NameInformation> | ref<Property> | ref<SignalWidth>)));
  EDIF_FORM(SignalAnnotate, "(signalAnnotate",
           arg(extendSignalDefExpr | extendSignalMemberDefExpr) +
           many(arg(ref<Comment> | ref<Property> | ref<PropertyOverride>)));
  EDIF_FORM(SignalGroup, "(signalGroup",
           arg(signalGroupNameDefExpr) +
           arg(ref<SignalList>) +
           many(arg(ref<Property> | ref<NameInformation>)));
  EDIF_FORM(SignalGroupAnnotate, "(signalGroupAnnotate",
           arg(extendSignalGroupDefExpr) +
           many(arg(ref<Comment> | ref<Property> | ref<PropertyOverride>)));
  EDIF_FORM(SignalGroupNameCaseSensitive, "(signalGroupNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(SignalGroupRef, "(signalGroupRef",
           arg(signalGroupNameRefExpr));
  EDIF_FORM0(SignalIndexValue, "(signalIndexValue");
  EDIF_FORM(SignalJoined, "(signalJoined",
           many(arg(ref<GlobalPortRef> | ref<PortInstanceRef> | ref<PortMemberRef> | ref<PortRef>)));
  EDIF_FORM(SignalList, "(signalList",
           many(arg(ref<SignalGroupRef> | ref<SignalRef>)));
  EDIF_FORM(SignalMemberRef, "(signalMemberRef",
           arg(signalNameRefExpr) +
           arg(ref<IndexValue>));
  EDIF_FORM(SignalNameCaseSensitive, "(signalNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(SignalRef, "(signalRef",
           arg(signalNameRefExpr));
  EDIF_FORM(SignalWidth, "(signalWidth",
           arg(integerExpressionExpr));
  EDIF_FORM(Status, "(status",
           many(arg(ref<Comment> | ref<Copyright> | ref<UserData> | ref<Written>)));
  EDIF_FORM(Step, "(step",
           arg(integerValueExpr));
  EDIF_FORM(String, "(string",
           arg(stringExpressionExpr));
  EDIF_FORM(StringConcatenate, "(stringConcatenate",
           many(arg(stringExpressionExpr)));
  EDIF_FORM(StringConstant, "(stringConstant",
           arg(constantNameDefExpr) +
           arg(stringTokenExpr));
  EDIF_FORM(StringConstantRef, "(stringConstantRef",
           arg(constantNameRefExpr));
  EDIF_FORM(StringEqual, "(stringEqual",
           arg(firstStringExpressionExpr) +
           arg(secondStringExpressionExpr));
  EDIF_FORM(StringLength, "(stringLength",
           arg(stringExpressionExpr));
  EDIF_FORM(StringParameter, "(stringParameter",
           arg(parameterNameDefExpr) +
           many(arg(ref<NameInformation> | ref<String>)));
  EDIF_FORM(StringParameterAssign, "(stringParameterAssign",
           arg(parameterNameRefExpr) +
           arg(stringExpressionExpr));
  EDIF_FORM(StringParameterRef, "(stringParameterRef",
           arg(parameterNameRefExpr));
  EDIF_FORM(StringPrefix, "(stringPrefix",
           arg(stringExpressionExpr) +
           arg(substringLengthExpr));
  EDIF_FORM(StringSuffix, "(stringSuffix",
           arg(stringExpressionExpr) +
           arg(substringOffsetExpr));
  EDIF_FORM(Strong, "(strong",
           arg(logicNameRefExpr));
  EDIF_FORM(Substring, "(substring",
           arg(stringExpressionExpr) +
           arg(substringLengthExpr) +
           arg(substringOffsetExpr));
  EDIF_FORM(SymbolicLayoutUnits, "(symbolicLayoutUnits",
           many(arg(ref<SetAngle> | ref<SetCapacitance> | ref<SetDistance> | ref<SetTime>)));
  EDIF_FORM(SymbolicLayoutView, "(symbolicLayoutView",
           arg(viewNameDefExpr) +
           many(arg(ref<Comment> | ref<UserData>)) +
           option(arg(ref<NameInformation>) +
                  many(arg(ref<Comment> | ref<UserData>))));
  EDIF_FORM(Technology, "(technology",
           arg(ref<PhysicalScaling>) +
           many(arg(ref<Comment> | ref<FigureGroup> | ref<LogicDefinitions> | ref<UserData>)));
  EDIF_FORM(TextHeight, "(textHeight",
           arg(lengthValueExpr));
  EDIF_FORM(Time, "(time",
           arg(hourNumberExpr) +
           arg(minuteNumberExpr) +
           arg(secondNumberExpr));
  EDIF_FORM(TimeInterval, "(timeInterval",
           arg(ref<Event> | ref<OffsetEvent>) +
           arg(ref<Event> | ref<OffsetEvent> | ref<Duration>));
  EDIF_FORM(TimeStamp, "(timeStamp",
           arg(ref<Date>) +
           arg(ref<Time>));
  EDIF_FORM(Timing, "(timing",
           arg(timingNameDefExpr) +
           arg(derivationExpr) +
           many(arg(ref<Comment> | ref<ForbiddenEvent> | ref<PathDelay> | ref<Property> | ref<UserData>)));
  EDIF_FORM(TimingDisplay, "(timingDisplay",
           arg(timingNameRefExpr) +
           many(arg(ref<Display>)));
  EDIF_FORM0(TopJustify, "(topJustify");
  EDIF_FORM(TotalPages, "(totalPages",
           arg(integerTokenExpr));
  EDIF_FORM(TotalPagesDisplay, "(totalPagesDisplay",
           arg(ref<AddDisplay> | ref<ReplaceDisplay> | ref<RemoveDisplay>));
  EDIF_FORM(Transform, "(transform",
           many(arg(ref<Origin> | ref<Rotation> | ref<ScaleX> | ref<ScaleY>)));
  EDIF_FORM(Transition, "(transition",
           arg(previousLogicValueExpr) +
           arg(presentLogicValueExpr));
  EDIF_FORM0(Etrue, "(true");
  EDIF_FORM0(Truncate, "(truncate");
  EDIF_FORM(Typeface, "(typeface",
           arg(fontFamilyExpr) +
           option(arg(ref<TypefaceSuffix>)));
  EDIF_FORM(TypefaceSuffix, "(typefaceSuffix",
           arg(stringTokenExpr));
  EDIF_FORM0(Unconfigured, "(unconfigured");
  EDIF_FORM0(Unconstrained, "(unconstrained");
  EDIF_FORM0(Undefined, "(undefined");
  EDIF_FORM(Unit, "(unit",
           arg(unitNameDefExpr) +
           arg(numberOfNewUnitsExpr) +
           arg(numberOfBasicUnitsExpr) +
           many(arg(ref<Ampere> | ref<Candela> | ref<Celsius> | ref<Coulomb> | ref<Degree> | ref<Fahrenheit> | ref<Farad> | ref<Henry> | ref<Hertz> | ref<Joule> | ref<Kelvin> | ref<Kilogram> | ref<Meter> | ref<Mole> | ref<Ohm> | ref<Radian> | ref<Second> | ref<Siemens> | ref<Volt> | ref<Watt> | ref<Weber>)));
  EDIF_FORM(UnitDefinitions, "(unitDefinitions",
           many(arg(ref<Unit>)));
  EDIF_FORM(UnitRef, "(unitRef",
           arg(unitNameRefExpr));
  EDIF_FORM0(Unrestricted, "(unrestricted");
  EDIF_FORM0(Unspecified, "(unspecified");
  EDIF_FORM0(UnspecifiedDirectionPort, "(unspecifiedDirectionPort");
  EDIF_FORM0(Untyped, "(untyped");
  EDIF_FORM0(Unused, "(unused");
  EDIF_FORM(UsableArea, "(usableArea",
           many(arg(ref<Rectangle>)));
  EDIF_FORM(UserData, "(userData",
           arg(userDataTagExpr) +
           many(arg(ref<IDENTIFIER> | integerTokenExpr | stringTokenExpr | ref<UserData>)));
  EDIF_FORM(Version, "(version",
           arg(stringValueExpr));
  EDIF_FORM(VerticalJustification, "(verticalJustification",
           arg(ref<BottomJustify> | ref<BaselineJustify> | ref<MiddleJustify> | ref<CaplineJustify> | ref<TopJustify>));
  EDIF_FORM(ViewGroup, "(viewGroup",
           arg(viewGroupNameDefExpr) +
           arg(ref<ViewGroupHeader>) +
           many(arg(ref<Comment> | ref<UserData> | ref<ViewGroupRef> | ref<ViewRef>)));
  EDIF_FORM(ViewGroupHeader, "(viewGroupHeader",
           many(arg(ref<Documentation> | ref<NameInformation> | ref<Property> | ref<Reason>)));
  EDIF_FORM(ViewGroupNameCaseSensitive, "(viewGroupNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(ViewGroupRef, "(viewGroupRef",
           arg(viewGroupNameRefExpr));
  EDIF_FORM(ViewNameCaseSensitive, "(viewNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(ViewNameDisplay, "(viewNameDisplay",
           many(arg(ref<Display> | ref<DisplayNameOverride>)));
  EDIF_FORM(ViewPropertyDisplay, "(viewPropertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<Display> | ref<PropertyNameDisplay>)));
  EDIF_FORM(ViewPropertyOverride, "(viewPropertyOverride",
           arg(propertyNameRefExpr) +
           arg(typedValueExpr | ref<Untyped>) +
           many(arg(ref<Comment> | ref<Fixed> | ref<PropertyOverride>)));
  EDIF_FORM(ViewRef, "(viewRef",
           arg(viewNameRefExpr) +
           option(arg(ref<ClusterRef>)));
  EDIF_FORM(Visible, "(visible",
           arg(booleanValueExpr));
  EDIF_FORM(Volt, "(volt",
           arg(unitExponentExpr));
  EDIF_FORM(VoltageMap, "(voltageMap",
           arg(voltageValueExpr));
  EDIF_FORM(Watt, "(watt",
           arg(unitExponentExpr));
  EDIF_FORM(Weak, "(weak",
           arg(logicNameRefExpr));
  EDIF_FORM(WeakJoined, "(weakJoined",
           many(arg(ref<InterfaceJoined> | ref<PortRef>)));
  EDIF_FORM(Weber, "(weber",
           arg(unitExponentExpr));
  EDIF_FORM0(WidePort, "(widePort");
  EDIF_FORM0(WideWire, "(wideWire");
  EDIF_FORM(Written, "(written",
           arg(ref<TimeStamp>) +
           many(arg(ref<Author> | ref<Comment> | ref<DataOrigin> | ref<Program> | ref<UserData>)));
  EDIF_FORM(Xor, "(xor",
           many(arg(booleanExpressionExpr)));
  EDIF_FORM(Year, "(year",
           many(arg(yearNumberExpr)));

#undef EDIF_FORM0
#undef EDIF_FORM

  // goal: edif EOF -- the whole document is one (edif ...) form.
  struct Goal : rule<ref<Edif> + eof> {};

  using entry = Goal;
  using ignored = WS;
};

}  // namespace edif300
