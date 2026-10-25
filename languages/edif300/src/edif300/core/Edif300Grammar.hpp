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
  using Unquoted = text_conv<unquote>;
  struct STRING_LITERAL
      : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">, Unquoted> {};

  // The fractional part is digits, not a repeated signed integer run: read
  // as the decimal number this spelling names -- every corpus value is a
  // plain (optionally signed) decimal.
  struct DECIMAL_LITERAL
      : terminal<option(cr<"+-">) + some(d) + option(lit<"."> + many(d)),
                 double> {};

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  static constexpr auto arg = append<&ast::Form::args>;

#define EDIF_FORM0(Name, Kw)                                          \
  struct Name : rule<assign<&ast::Form::keyword>(kw<Kw>) + lit<")">> {}
#define EDIF_FORM(Name, Kw, Body)                                \
  struct Name                                                    \
      : rule<assign<&ast::Form::keyword>(kw<Kw>) + (Body) + lit<")">> {}

  // Forward declarations: every construct may reference any other.

  struct AbsoluteRule;
  struct AcLoadRule;
  struct AcLoadDisplayRule;
  struct AddDisplayRule;
  struct AmpereRule;
  struct AndRule;
  struct AnnotateRule;
  struct ApprovedDateRule;
  struct ApprovedDateDisplayRule;
  struct ArcRule;
  struct AsciiRule;
  struct AssociatedInterconnectNameDisplayRule;
  struct AuthorRule;
  struct BackgroundColorRule;
  struct BaselineJustifyRule;
  struct BecomesRule;
  struct BehaviorViewRule;
  struct BidirectionalRule;
  struct BidirectionalPortRule;
  struct BidirectionalPortAttributesRule;
  struct BitOrderRule;
  struct BoldStyleRule;
  struct EbooleanRule;
  struct BooleanConstantRule;
  struct BooleanConstantRefRule;
  struct BooleanMapRule;
  struct BooleanParameterRule;
  struct BooleanParameterAssignRule;
  struct BooleanParameterRefRule;
  struct BorderPatternRule;
  struct BorderPatternVisibleRule;
  struct BorderWidthRule;
  struct BottomJustifyRule;
  struct CalculatedRule;
  struct CandelaRule;
  struct CaplineJustifyRule;
  struct CellRule;
  struct CellHeaderRule;
  struct CellNameDisplayRule;
  struct CellPropertyDisplayRule;
  struct CellPropertyDisplayOverrideRule;
  struct CellPropertyOverrideRule;
  struct CellRefRule;
  struct CelsiusRule;
  struct CenterJustifyRule;
  struct CharacterEncodingRule;
  struct CheckDateRule;
  struct CheckDateDisplayRule;
  struct CircleRule;
  struct ClusterRule;
  struct ClusterConfigurationRule;
  struct ClusterConfigurationNameCaseSensitiveRule;
  struct ClusterConfigurationRefRule;
  struct ClusterHeaderRule;
  struct ClusterNameCaseSensitiveRule;
  struct ClusterPropertyDisplayRule;
  struct ClusterPropertyDisplayOverrideRule;
  struct ClusterPropertyOverrideRule;
  struct ClusterRefRule;
  struct ColorRule;
  struct CommentRule;
  struct CommentGraphicsRule;
  struct CompanyNameRule;
  struct CompanyNameDisplayRule;
  struct ComplementedNameRule;
  struct ComplementedNamePartRule;
  struct ComplexGeometryRule;
  struct ComplexNameRule;
  struct CompoundRule;
  struct ConditionRule;
  struct ConditionDisplayRule;
  struct ConnectedSignalIndexGeneratorRule;
  struct ConnectedSignalIndexGeneratorDisplayRule;
  struct ConnectivityBusRule;
  struct ConnectivityBusJoinedRule;
  struct ConnectivityBusSliceRule;
  struct ConnectivityFrameStructureRule;
  struct ConnectivityNetRule;
  struct ConnectivityNetJoinedRule;
  struct ConnectivityRipperRule;
  struct ConnectivityRipperRefRule;
  struct ConnectivityStructureRule;
  struct ConnectivitySubBusRule;
  struct ConnectivitySubNetRule;
  struct ConnectivityTagGeneratorRule;
  struct ConnectivityTagGeneratorDisplayRule;
  struct ConnectivityUnitsRule;
  struct ConnectivityViewRule;
  struct ConnectivityViewHeaderRule;
  struct ConstantValuesRule;
  struct ContractRule;
  struct ContractDisplayRule;
  struct CopyrightRule;
  struct CopyrightDisplayRule;
  struct CornerTypeRule;
  struct CoulombRule;
  struct CriticalityRule;
  struct CriticalityDisplayRule;
  struct CurrentMapRule;
  struct CurveRule;
  struct DataOriginRule;
  struct DateRule;
  struct DcFanInLoadRule;
  struct DcFanInLoadDisplayRule;
  struct DcFanOutLoadRule;
  struct DcFanOutLoadDisplayRule;
  struct DcMaxFanInRule;
  struct DcMaxFanInDisplayRule;
  struct DcMaxFanOutRule;
  struct DcMaxFanOutDisplayRule;
  struct DecimalToStringRule;
  struct DefaultClusterConfigurationRule;
  struct DefaultConnectionRule;
  struct DegreeRule;
  struct DelayRule;
  struct DerivedFromRule;
  struct DesignRule;
  struct DesignatorRule;
  struct DesignatorDisplayRule;
  struct DesignHeaderRule;
  struct DesignHierarchyRule;
  struct DesignHierarchyHeaderRule;
  struct DesignHierarchyNameCaseSensitiveRule;
  struct DesignNameCaseSensitiveRule;
  struct DesignUnitsRule;
  struct DiagramRule;
  struct DirectionalPortAttributeOverrideRule;
  struct DisplayRule;
  struct DisplayAttributesRule;
  struct DisplayNameRule;
  struct DisplayNameOverrideRule;
  struct DocumentationRule;
  struct DocumentationHeaderRule;
  struct DocumentationNameCaseSensitiveRule;
  struct DocumentationUnitsRule;
  struct DominatesRule;
  struct DotRule;
  struct DrawingDescriptionRule;
  struct DrawingDescriptionDisplayRule;
  struct DrawingIdentificationRule;
  struct DrawingIdentificationDisplayRule;
  struct DrawingSizeRule;
  struct DrawingSizeDisplayRule;
  struct DurationRule;
  struct ERule;
  struct EdifRule;
  struct EdifHeaderRule;
  struct EdifLevelRule;
  struct EdifVersionRule;
  struct EndTypeRule;
  struct EngineeringDateRule;
  struct EngineeringDateDisplayRule;
  struct EventRule;
  struct ExtendRule;
  struct ExternalRule;
  struct FahrenheitRule;
  struct EfalseRule;
  struct FaradRule;
  struct FigureRule;
  struct FigureGroupRule;
  struct FigureGroupNameCaseSensitiveRule;
  struct FigureGroupOverrideRule;
  struct FillPatternRule;
  struct FillPatternVisibleRule;
  struct FixedRule;
  struct FontRule;
  struct FontCapitalHeightRule;
  struct FontDefinitionsRule;
  struct FontDescentRule;
  struct FontHeightRule;
  struct FontProportionsRule;
  struct FontRefRule;
  struct FontsRule;
  struct FontWidthRule;
  struct ForbiddenEventRule;
  struct ForFrameRule;
  struct ForFrameAnnotateRule;
  struct ForFrameIndexRule;
  struct ForFrameIndexDisplayRule;
  struct ForFrameIndexNameCaseSensitiveRule;
  struct ForFrameIndexRefRule;
  struct ForFrameMemberRefRule;
  struct ForFrameRefRule;
  struct FrameConfigurationRule;
  struct FrameNameCaseSensitiveRule;
  struct FrameRefRule;
  struct FromBottomRule;
  struct FromLeftRule;
  struct FromRightRule;
  struct FromTopRule;
  struct GeneratedRule;
  struct GeometryMacroRule;
  struct GeometryMacroHeaderRule;
  struct GeometryMacroRefRule;
  struct GeometryMacroUnitsRule;
  struct GlobalPortRule;
  struct GlobalPortBundleRule;
  struct GlobalPortDefinitionsRule;
  struct GlobalPortListRule;
  struct GlobalPortNameCaseSensitiveRule;
  struct GlobalPortNameDisplayRule;
  struct GlobalPortPropertyDisplayRule;
  struct GlobalPortRefRule;
  struct GlobalPortScopeRule;
  struct HenryRule;
  struct HertzRule;
  struct HorizontalJustificationRule;
  struct HotspotRule;
  struct HotspotConnectDirectionRule;
  struct HotspotGridRule;
  struct HotspotNameCaseSensitiveRule;
  struct HotspotNameDisplayRule;
  struct IeeeSectionRule;
  struct IeeeStandardRule;
  struct IfFrameRule;
  struct IfFrameAnnotateRule;
  struct IfFrameRefRule;
  struct IfFrameSetRule;
  struct IgnoreRule;
  struct ImplementationNameCaseSensitiveRule;
  struct ImplementationNameDisplayRule;
  struct IndexEndDisplayRule;
  struct IndexNameDisplayRule;
  struct IndexStartRule;
  struct IndexStartDisplayRule;
  struct IndexStepRule;
  struct IndexStepDisplayRule;
  struct IndexValueRule;
  struct InputRule;
  struct InputPortRule;
  struct InputPortAttributesRule;
  struct InstanceRule;
  struct InstanceConfigurationRule;
  struct InstanceIndexValueRule;
  struct InstanceMemberRefRule;
  struct InstanceNameCaseSensitiveRule;
  struct InstanceNameDisplayRule;
  struct InstanceNameGeneratorRule;
  struct InstanceNameGeneratorDisplayRule;
  struct InstancePortAttributeDisplayRule;
  struct InstancePortAttributesRule;
  struct InstancePropertyDisplayRule;
  struct InstancePropertyOverrideRule;
  struct InstanceRefRule;
  struct InstanceWidthRule;
  struct InstanceWidthDisplayRule;
  struct IntegerRule;
  struct IntegerConstantRule;
  struct IntegerConstantRefRule;
  struct IntegerEqualRule;
  struct IntegerParameterRule;
  struct IntegerParameterAssignRule;
  struct IntegerParameterRefRule;
  struct IntegerProductRule;
  struct IntegerQuotientRule;
  struct IntegerRemainderRule;
  struct IntegerSubtractRule;
  struct IntegerSumRule;
  struct InterconnectAnnotateRule;
  struct InterconnectAttachedTextRule;
  struct InterconnectDelayRule;
  struct InterconnectDelayDisplayRule;
  struct InterconnectHeaderRule;
  struct InterconnectNameCaseSensitiveRule;
  struct InterconnectNameDisplayRule;
  struct InterconnectPropertyDisplayRule;
  struct InterconnectRefRule;
  struct InterconnectSetRule;
  struct EinterfaceRule;
  struct InterfaceJoinedRule;
  struct InterfaceUnitsRule;
  struct Iso8859Rule;
  struct IsolatedRule;
  struct ItalicStyleRule;
  struct Jisx0201Rule;
  struct Jisx0208Rule;
  struct JoinedAsSignalRule;
  struct JouleRule;
  struct K0KeywordLevelRule;
  struct K1KeywordAliasRule;
  struct K1KeywordLevelRule;
  struct K2ActualRule;
  struct K2BuildRule;
  struct K2FormalRule;
  struct K2GenerateRule;
  struct K2KeywordDefineRule;
  struct K2KeywordLevelRule;
  struct K2KeywordParametersRule;
  struct K2LiteralRule;
  struct K2OptionalRule;
  struct K2RequiredRule;
  struct K3BuildRule;
  struct K3CollectorRule;
  struct K3ForEachRule;
  struct K3FormalRule;
  struct K3FormalListRule;
  struct K3GenerateRule;
  struct K3KeywordDefineRule;
  struct K3KeywordLevelRule;
  struct K3KeywordParametersRule;
  struct KelvinRule;
  struct KeywordMapRule;
  struct KilogramRule;
  struct LeafRule;
  struct LeafOccurrenceAnnotateRule;
  struct LeftJustifyRule;
  struct LessThanRule;
  struct LessThanOrEqualRule;
  struct LibraryRule;
  struct LibraryHeaderRule;
  struct LibraryNameCaseSensitiveRule;
  struct LibraryObjectNameCaseSensitiveRule;
  struct LibraryRefRule;
  struct LoadDelayRule;
  struct LocalPortGroupRule;
  struct LocalPortGroupNameCaseSensitiveRule;
  struct LocalPortGroupRefRule;
  struct LogicalConnectivityRule;
  struct LogicDefinitionsRule;
  struct LogicListRule;
  struct LogicMapInputRule;
  struct LogicMapOutputRule;
  struct LogicModelUnitsRule;
  struct LogicModelViewRule;
  struct LogicOneOfRule;
  struct LogicRefRule;
  struct LogicValueRule;
  struct LsbToMsbRule;
  struct MaskLayoutUnitsRule;
  struct MaskLayoutViewRule;
  struct MeasuredRule;
  struct MeterRule;
  struct MiddleJustifyRule;
  struct MinimalWidthRule;
  struct MinimumStringLengthRule;
  struct MiNoMaxRule;
  struct MixedDirectionRule;
  struct MnmRule;
  struct MoleRule;
  struct MsbToLsbRule;
  struct MustJoinRule;
  struct NameAliasRule;
  struct NameCaseSensitivityRule;
  struct NameDimensionRule;
  struct NameDimensionStructureRule;
  struct NameInformationRule;
  struct NamePartSeparatorRule;
  struct NameStructureRule;
  struct NarrowPortRule;
  struct NarrowWireRule;
  struct NoHotspotGridRule;
  struct NominalHotspotGridRule;
  struct NonPermutableRule;
  struct NotRule;
  struct NotInheritedRule;
  struct NumberRule;
  struct NumberParameterRule;
  struct NumberParameterAssignRule;
  struct NumberParameterRefRule;
  struct NumberPointRule;
  struct OccurrenceAnnotateRule;
  struct OccurrenceHierarchyAnnotateRule;
  struct OffsetEventRule;
  struct OhmRule;
  struct OpenShapeRule;
  struct OrRule;
  struct OriginRule;
  struct OriginalBoundingBoxRule;
  struct OriginalDrawingDateRule;
  struct OriginalDrawingDateDisplayRule;
  struct OtherwiseFrameRule;
  struct OtherwiseFrameAnnotateRule;
  struct OtherwiseFrameRefRule;
  struct OutputRule;
  struct OutputPortRule;
  struct OutputPortAttributesRule;
  struct OwnerRule;
  struct PageRule;
  struct PageAnnotateRule;
  struct PageBorderRule;
  struct PageBorderTemplateRule;
  struct PageBorderTemplateRefRule;
  struct PageCommentGraphicsRule;
  struct PageHeaderRule;
  struct PageIdentificationRule;
  struct PageIdentificationDisplayRule;
  struct PageNameCaseSensitiveRule;
  struct PagePropertyDisplayRule;
  struct PageRefRule;
  struct PageSizeRule;
  struct PageTitleRule;
  struct PageTitleBlockRule;
  struct PageTitleBlockAttributeDisplayRule;
  struct PageTitleBlockAttributesRule;
  struct PageTitleBlockTemplateRule;
  struct PageTitleBlockTemplateRefRule;
  struct PageTitleDisplayRule;
  struct ParameterDisplayRule;
  struct ParameterNameCaseSensitiveRule;
  struct ParameterNameDisplayRule;
  struct PathRule;
  struct PathDelayRule;
  struct PathWidthRule;
  struct PcbLayoutUnitsRule;
  struct PcbLayoutViewRule;
  struct PermutableRule;
  struct PhysicalDefaultsRule;
  struct PhysicalScalingRule;
  struct PixelPatternRule;
  struct PixelRowRule;
  struct PointRule;
  struct PointListRule;
  struct PolygonRule;
  struct PortRule;
  struct PortAnnotateRule;
  struct PortAttributeDisplayRule;
  struct PortBundleRule;
  struct PortDelayRule;
  struct PortDelayDisplayRule;
  struct PortDelayOverrideRule;
  struct PortIndexValueRule;
  struct PortInstanceRefRule;
  struct PortJoinedRule;
  struct PortListRule;
  struct PortLoadDelayRule;
  struct PortLoadDelayDisplayRule;
  struct PortLoadDelayOverrideRule;
  struct PortMemberRefRule;
  struct PortNameCaseSensitiveRule;
  struct PortNameDisplayRule;
  struct PortNameGeneratorRule;
  struct PortNameGeneratorDisplayRule;
  struct PortPropertyDisplayRule;
  struct PortPropertyDisplayOverrideRule;
  struct PortPropertyOverrideRule;
  struct PortRefRule;
  struct PortSetRule;
  struct PortWidthRule;
  struct PreviousVersionRule;
  struct PrimaryNameRule;
  struct ProgramRule;
  struct PropertyRule;
  struct PropertyDisplayRule;
  struct PropertyDisplayOverrideRule;
  struct PropertyInheritanceControlRule;
  struct PropertyNameCaseSensitiveRule;
  struct PropertyNameDisplayRule;
  struct PropertyOverrideRule;
  struct ProportionalFontRule;
  struct PtRule;
  struct RadianRule;
  struct ReasonRule;
  struct RectangleRule;
  struct RemoveDisplayRule;
  struct RepetitionCountRule;
  struct RepetitionCountDisplayRule;
  struct ReplaceDisplayRule;
  struct RequiredRule;
  struct ResolvesRule;
  struct RevisionRule;
  struct RevisionDisplayRule;
  struct RightJustifyRule;
  struct RipperHotspotRule;
  struct RipperHotspotRefRule;
  struct RotationRule;
  struct RoundRule;
  struct ScaleXRule;
  struct ScaleYRule;
  struct SchematicBusRule;
  struct SchematicBusDetailsRule;
  struct SchematicBusGraphicsRule;
  struct SchematicBusJoinedRule;
  struct SchematicBusSliceRule;
  struct SchematicComplexFigureRule;
  struct SchematicFigureMacroRule;
  struct SchematicFigureMacroRefRule;
  struct SchematicForFrameBorderRule;
  struct SchematicForFrameBorderTemplateRule;
  struct SchematicForFrameBorderTemplateRefRule;
  struct SchematicForFrameImplementationRule;
  struct SchematicForFrameImplementationHeaderRule;
  struct SchematicFrameImplementationDetailsRule;
  struct SchematicGlobalPortAttributesRule;
  struct SchematicGlobalPortImplementationRule;
  struct SchematicGlobalPortImplementationRefRule;
  struct SchematicGlobalPortTemplateRule;
  struct SchematicGlobalPortTemplateRefRule;
  struct SchematicIfFrameBorderRule;
  struct SchematicIfFrameBorderTemplateRule;
  struct SchematicIfFrameBorderTemplateRefRule;
  struct SchematicIfFrameImplementationRule;
  struct SchematicIfFrameImplementationHeaderRule;
  struct SchematicImplementationRule;
  struct SchematicInstanceImplementationRule;
  struct SchematicInstanceImplementationRefRule;
  struct SchematicInterconnectAttributeDisplayRule;
  struct SchematicInterconnectHeaderRule;
  struct SchematicInterconnectTerminatorImplementationRule;
  struct SchematicInterconnectTerminatorImplementationRefRule;
  struct SchematicInterconnectTerminatorTemplateRule;
  struct SchematicInterconnectTerminatorTemplateRefRule;
  struct SchematicJunctionImplementationRule;
  struct SchematicJunctionImplementationRefRule;
  struct SchematicJunctionTemplateRule;
  struct SchematicJunctionTemplateRefRule;
  struct SchematicMasterPortImplementationRule;
  struct SchematicMasterPortImplementationRefRule;
  struct SchematicMasterPortTemplateRule;
  struct SchematicMasterPortTemplateRefRule;
  struct SchematicMetricRule;
  struct SchematicNetRule;
  struct SchematicNetDetailsRule;
  struct SchematicNetGraphicsRule;
  struct SchematicNetJoinedRule;
  struct SchematicOffPageConnectorImplementationRule;
  struct SchematicOffPageConnectorImplementationRefRule;
  struct SchematicOffPageConnectorTemplateRule;
  struct SchematicOffPageConnectorTemplateRefRule;
  struct SchematicOnPageConnectorImplementationRule;
  struct SchematicOnPageConnectorImplementationRefRule;
  struct SchematicOnPageConnectorTemplateRule;
  struct SchematicOnPageConnectorTemplateRefRule;
  struct SchematicOtherwiseFrameBorderRule;
  struct SchematicOtherwiseFrameBorderTemplateRule;
  struct SchematicOtherwiseFrameBorderTemplateRefRule;
  struct SchematicOtherwiseFrameImplementationRule;
  struct SchematicOtherwiseFrameImplementationHeaderRule;
  struct SchematicPortAcPowerRule;
  struct SchematicPortAcPowerRecommendedFrequencyRule;
  struct SchematicPortAcPowerRecommendedVoltageRmsRule;
  struct SchematicPortAnalogRule;
  struct SchematicPortAttributesRule;
  struct SchematicPortChassisGroundRule;
  struct SchematicPortClockRule;
  struct SchematicPortDcPowerRule;
  struct SchematicPortDcPowerRecommendedVoltageRule;
  struct SchematicPortEarthGroundRule;
  struct SchematicPortGroundRule;
  struct SchematicPortNonLogicalRule;
  struct SchematicPortOpenCollectorRule;
  struct SchematicPortOpenEmitterRule;
  struct SchematicPortPowerRule;
  struct SchematicPortStyleRule;
  struct SchematicPortThreeStateRule;
  struct SchematicRequiredDefaultsRule;
  struct SchematicRipperImplementationRule;
  struct SchematicRipperImplementationRefRule;
  struct SchematicRipperTemplateRule;
  struct SchematicRipperTemplateRefRule;
  struct SchematicSubBusRule;
  struct SchematicSubBusSetRule;
  struct SchematicSubInterconnectHeaderRule;
  struct SchematicSubNetRule;
  struct SchematicSubNetSetRule;
  struct SchematicSymbolRule;
  struct SchematicSymbolBorderRule;
  struct SchematicSymbolBorderTemplateRule;
  struct SchematicSymbolBorderTemplateRefRule;
  struct SchematicSymbolHeaderRule;
  struct SchematicSymbolPortImplementationRule;
  struct SchematicSymbolPortImplementationRefRule;
  struct SchematicSymbolPortTemplateRule;
  struct SchematicSymbolPortTemplateRefRule;
  struct SchematicSymbolRefRule;
  struct SchematicTemplateHeaderRule;
  struct SchematicUnitsRule;
  struct SchematicViewRule;
  struct SchematicViewHeaderRule;
  struct SchematicWireAffinityRule;
  struct SchematicWireStyleRule;
  struct SecondRule;
  struct SectionRule;
  struct SectionTitleRule;
  struct SequenceRule;
  struct SetAngleRule;
  struct SetCapacitanceRule;
  struct SetCurrentRule;
  struct SetDistanceRule;
  struct SetFrequencyRule;
  struct SetTimeRule;
  struct SetVoltageRule;
  struct ShapeRule;
  struct SiemensRule;
  struct SignalRule;
  struct SignalAnnotateRule;
  struct SignalGroupRule;
  struct SignalGroupAnnotateRule;
  struct SignalGroupNameCaseSensitiveRule;
  struct SignalGroupRefRule;
  struct SignalIndexValueRule;
  struct SignalJoinedRule;
  struct SignalListRule;
  struct SignalMemberRefRule;
  struct SignalNameCaseSensitiveRule;
  struct SignalRefRule;
  struct SignalWidthRule;
  struct StatusRule;
  struct StepRule;
  struct StringRule;
  struct StringConcatenateRule;
  struct StringConstantRule;
  struct StringConstantRefRule;
  struct StringEqualRule;
  struct StringLengthRule;
  struct StringParameterRule;
  struct StringParameterAssignRule;
  struct StringParameterRefRule;
  struct StringPrefixRule;
  struct StringSuffixRule;
  struct StrongRule;
  struct SubstringRule;
  struct SymbolicLayoutUnitsRule;
  struct SymbolicLayoutViewRule;
  struct TechnologyRule;
  struct TextHeightRule;
  struct TimeRule;
  struct TimeIntervalRule;
  struct TimeStampRule;
  struct TimingRule;
  struct TimingDisplayRule;
  struct TopJustifyRule;
  struct TotalPagesRule;
  struct TotalPagesDisplayRule;
  struct TransformRule;
  struct TransitionRule;
  struct EtrueRule;
  struct TruncateRule;
  struct TypefaceRule;
  struct TypefaceSuffixRule;
  struct UnconfiguredRule;
  struct UnconstrainedRule;
  struct UndefinedRule;
  struct UnitRule;
  struct UnitDefinitionsRule;
  struct UnitRefRule;
  struct UnrestrictedRule;
  struct UnspecifiedRule;
  struct UnspecifiedDirectionPortRule;
  struct UntypedRule;
  struct UnusedRule;
  struct UsableAreaRule;
  struct UserDataRule;
  struct VersionRule;
  struct VerticalJustificationRule;
  struct ViewGroupRule;
  struct ViewGroupHeaderRule;
  struct ViewGroupNameCaseSensitiveRule;
  struct ViewGroupRefRule;
  struct ViewNameCaseSensitiveRule;
  struct ViewNameDisplayRule;
  struct ViewPropertyDisplayRule;
  struct ViewPropertyOverrideRule;
  struct ViewRefRule;
  struct VisibleRule;
  struct VoltRule;
  struct VoltageMapRule;
  struct WattRule;
  struct WeakRule;
  struct WeakJoinedRule;
  struct WeberRule;
  struct WidePortRule;
  struct WideWireRule;
  struct WrittenRule;
  struct XorRule;
  struct YearRule;

  // Type-union / pure-forwarding rules: shared expressions, not nodes

  // of their own -- spliced wherever the construct they stand for is named.

  static constexpr auto integerTokenExpr = ref<DECIMAL_LITERAL>;
  static constexpr auto scaledIntegerExpr = integerTokenExpr | ref<ERule>;
  static constexpr auto numberValueExpr = scaledIntegerExpr;
  static constexpr auto miNoMaxValueExpr = numberValueExpr | ref<MnmRule>;
  static constexpr auto capacitanceValueExpr = miNoMaxValueExpr;
  static constexpr auto acLoadFactorCapacitanceExpr = capacitanceValueExpr;
  static constexpr auto timeValueExpr = miNoMaxValueExpr;
  static constexpr auto acLoadFactorTimeExpr = timeValueExpr;
  static constexpr auto angleValueExpr = numberValueExpr;
  static constexpr auto pointValueExpr = ref<PtRule>;
  static constexpr auto attachmentPointExpr = pointValueExpr;
  static constexpr auto blueExpr = scaledIntegerExpr;
  static constexpr auto booleanTokenExpr = ref<EfalseRule> | ref<EtrueRule>;
  static constexpr auto booleanExpressionExpr = ref<AndRule> | ref<BooleanParameterRefRule> | booleanTokenExpr | ref<StringEqualRule> | ref<IntegerEqualRule> | ref<LessThanRule> | ref<LessThanOrEqualRule> | ref<NotRule> | ref<OrRule> | ref<XorRule> | ref<BooleanConstantRefRule>;
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
  static constexpr auto derivationExpr = ref<CalculatedRule> | ref<MeasuredRule> | ref<RequiredRule>;
  static constexpr auto designHierarchyNameDefExpr = nameDefExpr;
  static constexpr auto designNameDefExpr = nameDefExpr;
  static constexpr auto diagramNameDefExpr = nameDefExpr;
  static constexpr auto distanceValueExpr = integerValueExpr;
  static constexpr auto integerExpressionExpr = ref<IntegerParameterRefRule> | integerTokenExpr | ref<IntegerProductRule> | ref<IntegerSubtractRule> | ref<IntegerSumRule> | ref<IntegerRemainderRule> | ref<IntegerQuotientRule> | ref<StringLengthRule> | ref<IntegerConstantRefRule> | ref<ForFrameIndexRefRule> | ref<PortIndexValueRule> | ref<SignalIndexValueRule> | ref<AbsoluteRule> | ref<InstanceIndexValueRule>;
  static constexpr auto dividendExpr = integerExpressionExpr;
  static constexpr auto divisorExpr = integerExpressionExpr;
  static constexpr auto documentationNameDefExpr = nameDefExpr;
  static constexpr auto edifLevelValueExpr = integerTokenExpr;
  static constexpr auto edifNameDefExpr = nameDefExpr;
  static constexpr auto endPointExpr = pointValueExpr;
  static constexpr auto exponentExpr = integerTokenExpr;
  static constexpr auto extendForFrameMemberDefExpr = ref<ForFrameMemberRefRule>;
  static constexpr auto frameNameRefExpr = nameRefExpr;
  static constexpr auto extendFrameDefExpr = frameNameRefExpr;
  static constexpr auto instanceNameRefExpr = nameRefExpr;
  static constexpr auto extendInstanceDefExpr = instanceNameRefExpr;
  static constexpr auto extendInstanceMemberDefExpr = ref<InstanceMemberRefRule>;
  static constexpr auto interconnectNameRefExpr = nameRefExpr;
  static constexpr auto extendInterconnectDefExpr = interconnectNameRefExpr;
  static constexpr auto pageNameRefExpr = nameRefExpr;
  static constexpr auto extendPageDefExpr = pageNameRefExpr;
  static constexpr auto portNameRefExpr = nameRefExpr;
  static constexpr auto extendPortDefExpr = portNameRefExpr;
  static constexpr auto extendPortMemberDefExpr = ref<PortMemberRefRule>;
  static constexpr auto signalNameRefExpr = nameRefExpr;
  static constexpr auto extendSignalDefExpr = signalNameRefExpr;
  static constexpr auto signalGroupNameRefExpr = nameRefExpr;
  static constexpr auto extendSignalGroupDefExpr = signalGroupNameRefExpr;
  static constexpr auto extendSignalMemberDefExpr = ref<SignalMemberRefRule>;
  static constexpr auto figureGroupNameDefExpr = nameDefExpr;
  static constexpr auto figureGroupNameRefExpr = nameRefExpr;
  static constexpr auto firstIntegerExpressionExpr = integerExpressionExpr;
  static constexpr auto stringTokenExpr = ref<STRING_LITERAL>;
  static constexpr auto stringExpressionExpr = ref<StringParameterRefRule> | stringTokenExpr | ref<StringConstantRefRule> | ref<StringConcatenateRule> | ref<SubstringRule> | ref<DecimalToStringRule> | ref<StringPrefixRule> | ref<StringSuffixRule>;
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
  static constexpr auto numberExpressionExpr = numberValueExpr | ref<NumberParameterRefRule>;
  static constexpr auto numberOfBasicUnitsExpr = scaledIntegerExpr;
  static constexpr auto numberOfNewUnitsExpr = scaledIntegerExpr;
  static constexpr auto numeratorExpr = integerValueExpr;
  static constexpr auto originalNameExpr = stringTokenExpr;
  static constexpr auto pageNameDefExpr = nameDefExpr;
  static constexpr auto parameterNameDefExpr = nameDefExpr;
  static constexpr auto parameterNameRefExpr = nameRefExpr;
  static constexpr auto portDelayNameDefExpr = nameDefExpr;
  static constexpr auto portDelayNameRefExpr = nameRefExpr;
  static constexpr auto portDirectionExpr = ref<InputPortRule> | ref<OutputPortRule> | ref<BidirectionalPortRule> | ref<UnspecifiedDirectionPortRule>;
  static constexpr auto portDirectionIndicatorExpr = ref<InputRule> | ref<OutputRule> | ref<BidirectionalRule> | ref<UnspecifiedRule> | ref<UnrestrictedRule> | ref<MixedDirectionRule>;
  static constexpr auto portLoadDelayNameDefExpr = nameDefExpr;
  static constexpr auto portLoadDelayNameRefExpr = nameRefExpr;
  static constexpr auto portNameDefExpr = nameDefExpr;
  static constexpr auto presentLogicValueExpr = logicNameRefExpr | ref<LogicListRule> | ref<LogicOneOfRule>;
  static constexpr auto previousLogicValueExpr = logicNameRefExpr | ref<LogicListRule> | ref<LogicOneOfRule>;
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
  static constexpr auto throughPointExpr = ref<NumberPointRule>;
  static constexpr auto timeDelayExpr = timeValueExpr;
  static constexpr auto timingNameDefExpr = nameDefExpr;
  static constexpr auto timingNameRefExpr = nameRefExpr;
  static constexpr auto toIntegerExpr = integerTokenExpr;
  static constexpr auto typedValueExpr = ref<EbooleanRule> | ref<IntegerRule> | ref<MiNoMaxRule> | ref<NumberRule> | ref<PointRule> | ref<StringRule>;
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

  EDIF_FORM(AbsoluteRule, "(absolute",
           arg(integerExpressionExpr));
  EDIF_FORM(AcLoadRule, "(acLoad",
           arg(capacitanceValueExpr));
  EDIF_FORM(AcLoadDisplayRule, "(acLoadDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(AddDisplayRule, "(addDisplay",
           many(arg(ref<DisplayRule>)));
  EDIF_FORM(AmpereRule, "(ampere",
           arg(unitExponentExpr));
  EDIF_FORM(AndRule, "(and",
           many(arg(booleanExpressionExpr)));
  EDIF_FORM(AnnotateRule, "(annotate",
           arg(stringValueExpr) +
           many(arg(ref<DisplayRule>)));
  EDIF_FORM(ApprovedDateRule, "(approvedDate",
           arg(ref<DateRule>));
  EDIF_FORM(ApprovedDateDisplayRule, "(approvedDateDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(ArcRule, "(arc",
           arg(startPointExpr) +
           arg(throughPointExpr) +
           arg(endPointExpr));
  EDIF_FORM0(AsciiRule, "(ascii");
  EDIF_FORM(AssociatedInterconnectNameDisplayRule, "(associatedInterconnectNameDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(AuthorRule, "(author",
           arg(stringTokenExpr));
  EDIF_FORM(BackgroundColorRule, "(backgroundColor",
           arg(ref<ColorRule>));
  EDIF_FORM0(BaselineJustifyRule, "(baselineJustify");
  EDIF_FORM(BecomesRule, "(becomes",
           arg(logicNameRefExpr | ref<LogicListRule> | ref<LogicOneOfRule>));
  EDIF_FORM(BehaviorViewRule, "(behaviorView",
           arg(viewNameDefExpr) +
           many(arg(ref<CommentRule> | ref<NameInformationRule> | ref<UserDataRule>)));
  EDIF_FORM0(BidirectionalRule, "(bidirectional");
  EDIF_FORM(BidirectionalPortRule, "(bidirectionalPort",
           option(arg(ref<BidirectionalPortAttributesRule>)));
  EDIF_FORM(BidirectionalPortAttributesRule, "(bidirectionalPortAttributes",
           many(arg(ref<DcFanInLoadRule> | ref<DcFanOutLoadRule> | ref<DcMaxFanInRule> | ref<DcMaxFanOutRule>)));
  EDIF_FORM(BitOrderRule, "(bitOrder",
           arg(ref<LsbToMsbRule> | ref<MsbToLsbRule>));
  EDIF_FORM0(BoldStyleRule, "(boldStyle");
  EDIF_FORM(EbooleanRule, "(boolean",
           arg(booleanExpressionExpr));
  EDIF_FORM(BooleanConstantRule, "(booleanConstant",
           arg(constantNameDefExpr) +
           arg(booleanTokenExpr));
  EDIF_FORM(BooleanConstantRefRule, "(booleanConstantRef",
           arg(constantNameRefExpr));
  EDIF_FORM(BooleanMapRule, "(booleanMap",
           arg(booleanValueExpr));
  EDIF_FORM(BooleanParameterRule, "(booleanParameter",
           arg(parameterNameDefExpr) +
           many(arg(ref<EbooleanRule> | ref<NameInformationRule>)));
  EDIF_FORM(BooleanParameterAssignRule, "(booleanParameterAssign",
           arg(parameterNameRefExpr) +
           arg(booleanExpressionExpr));
  EDIF_FORM(BooleanParameterRefRule, "(booleanParameterRef",
           arg(parameterNameRefExpr));
  EDIF_FORM(BorderPatternRule, "(borderPattern",
           arg(ref<PixelPatternRule>));
  EDIF_FORM(BorderPatternVisibleRule, "(borderPatternVisible",
           arg(booleanValueExpr));
  EDIF_FORM(BorderWidthRule, "(borderWidth",
           arg(lengthValueExpr | ref<MinimalWidthRule>));
  EDIF_FORM0(BottomJustifyRule, "(bottomJustify");
  EDIF_FORM0(CalculatedRule, "(calculated");
  EDIF_FORM(CandelaRule, "(candela",
           arg(unitExponentExpr));
  EDIF_FORM0(CaplineJustifyRule, "(caplineJustify");
  EDIF_FORM(CellRule, "(cell",
           arg(libraryObjectNameDefExpr) +
           arg(ref<CellHeaderRule>) +
           many(arg(ref<ClusterRule> | ref<CommentRule> | ref<UserDataRule> | ref<ViewGroupRule>)));
  EDIF_FORM(CellHeaderRule, "(cellHeader",
           many(arg(ref<DocumentationRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<StatusRule>)));
  EDIF_FORM(CellNameDisplayRule, "(cellNameDisplay",
           many(arg(ref<DisplayRule> | ref<DisplayNameOverrideRule>)));
  EDIF_FORM(CellPropertyDisplayRule, "(cellPropertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<DisplayRule> | ref<PropertyNameDisplayRule>)));
  EDIF_FORM(CellPropertyDisplayOverrideRule, "(cellPropertyDisplayOverride",
           arg(propertyNameRefExpr) +
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>) +
           option(arg(ref<PropertyNameDisplayRule>)));
  EDIF_FORM(CellPropertyOverrideRule, "(cellPropertyOverride",
           arg(propertyNameRefExpr) +
           arg(typedValueExpr | ref<UntypedRule>) +
           many(arg(ref<CommentRule> | ref<FixedRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(CellRefRule, "(cellRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(CelsiusRule, "(celsius",
           arg(unitExponentExpr));
  EDIF_FORM0(CenterJustifyRule, "(centerJustify");
  EDIF_FORM(CharacterEncodingRule, "(characterEncoding",
           arg(ref<AsciiRule> | ref<Iso8859Rule> | ref<Jisx0201Rule> | ref<Jisx0208Rule>));
  EDIF_FORM(CheckDateRule, "(checkDate",
           arg(ref<DateRule>));
  EDIF_FORM(CheckDateDisplayRule, "(checkDateDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(CircleRule, "(circle",
           arg(pt1Expr) +
           arg(pt2Expr));
  EDIF_FORM(ClusterRule, "(cluster",
           arg(clusterNameDefExpr) +
           arg(ref<EinterfaceRule>) +
           arg(ref<ClusterHeaderRule>) +
           many(arg(ref<SchematicSymbolRule> | ref<SchematicViewRule> | ref<BehaviorViewRule> | ref<ClusterConfigurationRule> | ref<CommentRule> | ref<ConnectivityViewRule> | ref<DefaultClusterConfigurationRule> | ref<UserDataRule> | ref<LogicModelViewRule> | ref<MaskLayoutViewRule> | ref<PcbLayoutViewRule> | ref<SymbolicLayoutViewRule>)));
  EDIF_FORM(ClusterConfigurationRule, "(clusterConfiguration",
           arg(clusterConfigurationNameDefExpr) +
           arg(ref<ViewRefRule> | ref<LeafRule> | ref<UnconfiguredRule>) +
           many(arg(ref<CommentRule> | ref<FrameConfigurationRule> | ref<GlobalPortScopeRule> | ref<NameInformationRule> | ref<InstanceConfigurationRule> | ref<PropertyRule> | ref<UserDataRule>)));
  EDIF_FORM(ClusterConfigurationNameCaseSensitiveRule, "(clusterConfigurationNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(ClusterConfigurationRefRule, "(clusterConfigurationRef",
           arg(clusterConfigurationNameRefExpr));
  EDIF_FORM(ClusterHeaderRule, "(clusterHeader",
           many(arg(ref<DocumentationRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<StatusRule>)));
  EDIF_FORM(ClusterNameCaseSensitiveRule, "(clusterNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(ClusterPropertyDisplayRule, "(clusterPropertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<DisplayRule> | ref<PropertyNameDisplayRule>)));
  EDIF_FORM(ClusterPropertyDisplayOverrideRule, "(clusterPropertyDisplayOverride",
           arg(propertyNameRefExpr) +
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>) +
           option(arg(ref<PropertyNameDisplayRule>)));
  EDIF_FORM(ClusterPropertyOverrideRule, "(clusterPropertyOverride",
           arg(propertyNameRefExpr) +
           arg(typedValueExpr | ref<UntypedRule>) +
           many(arg(ref<CommentRule> | ref<FixedRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(ClusterRefRule, "(clusterRef",
           arg(clusterNameRefExpr) +
           option(arg(ref<CellRefRule>)));
  EDIF_FORM(ColorRule, "(color",
           arg(redExpr) +
           arg(greenExpr) +
           arg(blueExpr));
  EDIF_FORM(CommentRule, "(comment",
           many(arg(stringTokenExpr)));
  EDIF_FORM(CommentGraphicsRule, "(commentGraphics",
           many(arg(ref<AnnotateRule> | ref<CommentRule> | ref<FigureRule> | ref<OriginalBoundingBoxRule> | ref<PropertyDisplayRule> | ref<UserDataRule>)));
  EDIF_FORM(CompanyNameRule, "(companyName",
           arg(stringTokenExpr));
  EDIF_FORM(CompanyNameDisplayRule, "(companyNameDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(ComplementedNameRule, "(complementedName",
           many(arg(ref<ComplementedNamePartRule> | ref<NameDimensionRule> | ref<NamePartSeparatorRule> | simpleNameExpr)));
  EDIF_FORM(ComplementedNamePartRule, "(complementedNamePart",
           many(arg(ref<ComplementedNamePartRule> | ref<NamePartSeparatorRule> | simpleNameExpr)));
  EDIF_FORM(ComplexGeometryRule, "(complexGeometry",
           arg(ref<GeometryMacroRefRule>) +
           arg(ref<TransformRule>));
  EDIF_FORM(ComplexNameRule, "(complexName",
           many(arg(ref<ComplementedNamePartRule> | ref<NameDimensionRule> | ref<NamePartSeparatorRule> | simpleNameExpr)));
  EDIF_FORM(CompoundRule, "(compound",
           many(arg(logicNameRefExpr)));
  EDIF_FORM(ConditionRule, "(condition",
           arg(booleanExpressionExpr));
  EDIF_FORM(ConditionDisplayRule, "(conditionDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(ConnectedSignalIndexGeneratorRule, "(connectedSignalIndexGenerator",
           arg(integerExpressionExpr));
  EDIF_FORM(ConnectedSignalIndexGeneratorDisplayRule, "(connectedSignalIndexGeneratorDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(ConnectivityBusRule, "(connectivityBus",
           arg(interconnectNameDefExpr) +
           arg(ref<SignalGroupRefRule>) +
           arg(ref<InterconnectHeaderRule>) +
           arg(ref<ConnectivityBusJoinedRule>) +
           many(arg(ref<CommentRule> | ref<ConnectivityBusSliceRule> | ref<ConnectivitySubBusRule> | ref<UserDataRule>)));
  EDIF_FORM(ConnectivityBusJoinedRule, "(connectivityBusJoined",
           arg(ref<PortJoinedRule>) +
           many(arg(ref<ConnectivityRipperRefRule>)));
  EDIF_FORM(ConnectivityBusSliceRule, "(connectivityBusSlice",
           arg(interconnectNameDefExpr) +
           arg(ref<SignalGroupRefRule>) +
           arg(ref<InterconnectHeaderRule>) +
           arg(ref<ConnectivityBusJoinedRule>) +
           many(arg(ref<CommentRule> | ref<ConnectivityBusSliceRule> | ref<ConnectivitySubBusRule> | ref<UserDataRule>)));
  EDIF_FORM(ConnectivityFrameStructureRule, "(connectivityFrameStructure",
           arg(connectivityFrameStructureNameDefExpr) +
           arg(ref<FrameRefRule>) +
           many(arg(ref<CommentRule> | ref<ConnectivityBusRule> | ref<ConnectivityFrameStructureRule> | ref<ConnectivityNetRule> | ref<ConnectivityRipperRule> | ref<TimingRule> | ref<UserDataRule>)));
  EDIF_FORM(ConnectivityNetRule, "(connectivityNet",
           arg(interconnectNameDefExpr) +
           arg(ref<SignalRefRule>) +
           arg(ref<InterconnectHeaderRule>) +
           arg(ref<ConnectivityNetJoinedRule>) +
           many(arg(ref<CommentRule> | ref<ConnectivitySubNetRule> | ref<UserDataRule>)));
  EDIF_FORM(ConnectivityNetJoinedRule, "(connectivityNetJoined",
           arg(ref<PortJoinedRule> | ref<JoinedAsSignalRule>) +
           many(arg(ref<ConnectivityRipperRefRule>)));
  EDIF_FORM(ConnectivityRipperRule, "(connectivityRipper",
           arg(connectivityRipperNameDefExpr));
  EDIF_FORM(ConnectivityRipperRefRule, "(connectivityRipperRef",
           arg(connectivityRipperNameRefExpr));
  EDIF_FORM(ConnectivityStructureRule, "(connectivityStructure",
           many(arg(ref<CommentRule> | ref<ConnectivityBusRule> | ref<ConnectivityFrameStructureRule> | ref<ConnectivityNetRule> | ref<ConnectivityRipperRule> | ref<LocalPortGroupRule> | ref<TimingRule> | ref<UserDataRule>)));
  EDIF_FORM(ConnectivitySubBusRule, "(connectivitySubBus",
           arg(interconnectNameDefExpr) +
           arg(ref<InterconnectHeaderRule>) +
           arg(ref<ConnectivityBusJoinedRule>) +
           many(arg(ref<CommentRule> | ref<ConnectivityBusSliceRule> | ref<ConnectivitySubBusRule> | ref<UserDataRule>)));
  EDIF_FORM(ConnectivitySubNetRule, "(connectivitySubNet",
           arg(interconnectNameDefExpr) +
           arg(ref<InterconnectHeaderRule>) +
           arg(ref<ConnectivityNetJoinedRule>) +
           many(arg(ref<CommentRule> | ref<ConnectivitySubNetRule> | ref<UserDataRule>)));
  EDIF_FORM(ConnectivityTagGeneratorRule, "(connectivityTagGenerator",
           arg(stringExpressionExpr));
  EDIF_FORM(ConnectivityTagGeneratorDisplayRule, "(connectivityTagGeneratorDisplay",
           many(arg(ref<DisplayRule>)));
  EDIF_FORM(ConnectivityUnitsRule, "(connectivityUnits",
           option(arg(ref<SetCapacitanceRule> | ref<SetTimeRule>)));
  EDIF_FORM(ConnectivityViewRule, "(connectivityView",
           arg(viewNameDefExpr) +
           arg(ref<ConnectivityViewHeaderRule>) +
           arg(ref<LogicalConnectivityRule>) +
           arg(ref<ConnectivityStructureRule>) +
           many(arg(ref<CommentRule> | ref<UserDataRule>)));
  EDIF_FORM(ConnectivityViewHeaderRule, "(connectivityViewHeader",
           arg(ref<ConnectivityUnitsRule>) +
           many(arg(ref<DerivedFromRule> | ref<DocumentationRule> | ref<NameInformationRule> | ref<PreviousVersionRule> | ref<PropertyRule> | ref<StatusRule>)));
  EDIF_FORM(ConstantValuesRule, "(constantValues",
           many(arg(ref<BooleanConstantRule> | ref<IntegerConstantRule> | ref<StringConstantRule>)));
  EDIF_FORM(ContractRule, "(contract",
           arg(stringTokenExpr));
  EDIF_FORM(ContractDisplayRule, "(contractDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(CopyrightRule, "(copyright",
           arg(ref<YearRule>) +
           many(arg(stringTokenExpr)));
  EDIF_FORM(CopyrightDisplayRule, "(copyrightDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(CornerTypeRule, "(cornerType",
           arg(ref<TruncateRule> | ref<RoundRule> | ref<ExtendRule>));
  EDIF_FORM(CoulombRule, "(coulomb",
           arg(unitExponentExpr));
  EDIF_FORM(CriticalityRule, "(criticality",
           arg(integerValueExpr));
  EDIF_FORM(CriticalityDisplayRule, "(criticalityDisplay",
           many(arg(ref<DisplayRule>)));
  EDIF_FORM(CurrentMapRule, "(currentMap",
           arg(currentValueExpr));
  EDIF_FORM(CurveRule, "(curve",
           many(arg(ref<ArcRule> | pointValueExpr)));
  EDIF_FORM(DataOriginRule, "(dataOrigin",
           arg(stringTokenExpr) +
           option(arg(ref<VersionRule>)));
  EDIF_FORM(DateRule, "(date",
           arg(yearNumberExpr) +
           arg(monthNumberExpr) +
           arg(dayNumberExpr));
  EDIF_FORM(DcFanInLoadRule, "(dcFanInLoad",
           arg(numberValueExpr));
  EDIF_FORM(DcFanInLoadDisplayRule, "(dcFanInLoadDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(DcFanOutLoadRule, "(dcFanOutLoad",
           arg(numberValueExpr));
  EDIF_FORM(DcFanOutLoadDisplayRule, "(dcFanOutLoadDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(DcMaxFanInRule, "(dcMaxFanIn",
           arg(numberValueExpr));
  EDIF_FORM(DcMaxFanInDisplayRule, "(dcMaxFanInDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(DcMaxFanOutRule, "(dcMaxFanOut",
           arg(numberValueExpr));
  EDIF_FORM(DcMaxFanOutDisplayRule, "(dcMaxFanOutDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(DecimalToStringRule, "(decimalToString",
           arg(integerExpressionExpr) +
           option(arg(ref<MinimumStringLengthRule>)));
  EDIF_FORM(DefaultClusterConfigurationRule, "(defaultClusterConfiguration",
           arg(clusterConfigurationNameRefExpr));
  EDIF_FORM(DefaultConnectionRule, "(defaultConnection",
           arg(ref<GlobalPortRefRule>));
  EDIF_FORM(DegreeRule, "(degree",
           arg(unitExponentExpr));
  EDIF_FORM(DelayRule, "(delay",
           arg(timeDelayExpr));
  EDIF_FORM(DerivedFromRule, "(derivedFrom",
           arg(ref<ViewRefRule>) +
           option(arg(ref<ReasonRule>)));
  EDIF_FORM(DesignRule, "(design",
           arg(designNameDefExpr) +
           arg(ref<CellRefRule>) +
           arg(ref<DesignHeaderRule>) +
           many(arg(ref<CommentRule> | ref<DesignHierarchyRule> | ref<UserDataRule>)));
  EDIF_FORM(DesignatorRule, "(designator",
           arg(stringValueExpr));
  EDIF_FORM(DesignatorDisplayRule, "(designatorDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(DesignHeaderRule, "(designHeader",
           arg(ref<DesignUnitsRule>) +
           many(arg(ref<DocumentationRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<StatusRule>)));
  EDIF_FORM(DesignHierarchyRule, "(designHierarchy",
           arg(designHierarchyNameDefExpr) +
           arg(ref<ClusterRefRule>) +
           arg(ref<ClusterConfigurationRefRule>) +
           arg(ref<DesignHierarchyHeaderRule>) +
           option(arg(ref<OccurrenceHierarchyAnnotateRule>)));
  EDIF_FORM(DesignHierarchyHeaderRule, "(designHierarchyHeader",
           many(arg(ref<BooleanParameterAssignRule> | ref<IntegerParameterAssignRule> | ref<NameInformationRule> | ref<NumberParameterAssignRule> | ref<PropertyRule> | ref<StringParameterAssignRule>)));
  EDIF_FORM(DesignHierarchyNameCaseSensitiveRule, "(designHierarchyNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(DesignNameCaseSensitiveRule, "(designNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(DesignUnitsRule, "(designUnits",
           many(arg(ref<SetCapacitanceRule> | ref<SetTimeRule>)));
  EDIF_FORM(DiagramRule, "(diagram",
           arg(diagramNameDefExpr) +
           many(arg(ref<AnnotateRule> | ref<CommentRule> | ref<FigureRule> | ref<UserDataRule>)));
  EDIF_FORM(DirectionalPortAttributeOverrideRule, "(directionalPortAttributeOverride",
           arg(ref<InputPortAttributesRule> | ref<OutputPortAttributesRule> | ref<BidirectionalPortAttributesRule>));
  EDIF_FORM(DisplayRule, "(display",
           many(arg(figureGroupNameRefExpr | ref<FigureGroupOverrideRule>)) +
           arg(ref<TransformRule>));
  EDIF_FORM(DisplayAttributesRule, "(displayAttributes",
           many(arg(ref<BorderPatternRule> | ref<BorderPatternVisibleRule> | ref<BorderWidthRule> | ref<ColorRule> | ref<FillPatternRule> | ref<FillPatternVisibleRule> | ref<FontRefRule> | ref<HorizontalJustificationRule> | ref<TextHeightRule> | ref<VerticalJustificationRule> | ref<VisibleRule>)));
  EDIF_FORM(DisplayNameRule, "(displayName",
           arg(stringTokenExpr));
  EDIF_FORM(DisplayNameOverrideRule, "(displayNameOverride",
           arg(stringTokenExpr) +
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(DocumentationRule, "(documentation",
           arg(documentationNameDefExpr) +
           arg(ref<DocumentationHeaderRule>) +
           many(arg(ref<SectionRule>)));
  EDIF_FORM(DocumentationHeaderRule, "(documentationHeader",
           arg(ref<DocumentationUnitsRule>) +
           many(arg(ref<BackgroundColorRule> | ref<NameInformationRule> | ref<StatusRule>)));
  EDIF_FORM(DocumentationNameCaseSensitiveRule, "(documentationNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(DocumentationUnitsRule, "(documentationUnits",
           many(arg(ref<SetAngleRule> | ref<SetDistanceRule>)));
  EDIF_FORM(DominatesRule, "(dominates",
           many(arg(logicNameRefExpr)));
  EDIF_FORM(DotRule, "(dot",
           arg(pointValueExpr));
  EDIF_FORM(DrawingDescriptionRule, "(drawingDescription",
           arg(stringTokenExpr));
  EDIF_FORM(DrawingDescriptionDisplayRule, "(drawingDescriptionDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(DrawingIdentificationRule, "(drawingIdentification",
           arg(stringTokenExpr));
  EDIF_FORM(DrawingIdentificationDisplayRule, "(drawingIdentificationDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(DrawingSizeRule, "(drawingSize",
           arg(stringTokenExpr));
  EDIF_FORM(DrawingSizeDisplayRule, "(drawingSizeDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(DurationRule, "(duration",
           arg(timeValueExpr));
  EDIF_FORM(ERule, "(e",
           arg(mantissaExpr) +
           arg(exponentExpr));
  EDIF_FORM(EdifRule, "(edif",
           arg(edifNameDefExpr) +
           arg(ref<EdifVersionRule>) +
           arg(ref<EdifHeaderRule>) +
           many(arg(ref<LibraryRule> | ref<DesignRule> | ref<CommentRule> | ref<ExternalRule> | ref<UserDataRule>)));
  EDIF_FORM(EdifHeaderRule, "(edifHeader",
           arg(ref<EdifLevelRule>) +
           arg(ref<KeywordMapRule>) +
           arg(ref<UnitDefinitionsRule>) +
           arg(ref<FontDefinitionsRule>) +
           arg(ref<PhysicalDefaultsRule>) +
           many(arg(ref<CharacterEncodingRule> | ref<ConstantValuesRule> | ref<DocumentationRule> | ref<GlobalPortDefinitionsRule> | ref<NameCaseSensitivityRule> | ref<NameInformationRule> | ref<PhysicalScalingRule> | ref<PropertyRule> | ref<StatusRule>)));
  EDIF_FORM(EdifLevelRule, "(edifLevel",
           arg(edifLevelValueExpr));
  EDIF_FORM(EdifVersionRule, "(edifVersion",
           arg(markExpr) +
           arg(issueExpr) +
           arg(subIssueExpr));
  EDIF_FORM(EndTypeRule, "(endType",
           arg(ref<TruncateRule> | ref<RoundRule> | ref<ExtendRule>));
  EDIF_FORM(EngineeringDateRule, "(engineeringDate",
           arg(ref<DateRule>));
  EDIF_FORM(EngineeringDateDisplayRule, "(engineeringDateDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(EventRule, "(event",
           arg(ref<PortRefRule> | ref<PortListRule> | ref<PortSetRule> | ref<InterconnectRefRule> | ref<InterconnectSetRule>) +
           many(arg(ref<BecomesRule> | ref<TransitionRule>)));
  EDIF_FORM0(ExtendRule, "(extend");
  EDIF_FORM(ExternalRule, "(external",
           arg(libraryNameDefExpr) +
           arg(ref<LibraryHeaderRule>) +
           many(arg(ref<CellRule> | ref<CommentRule> | ref<GeometryMacroRule> | ref<PageBorderTemplateRule> | ref<PageTitleBlockTemplateRule> | ref<SchematicFigureMacroRule> | ref<SchematicForFrameBorderTemplateRule> | ref<SchematicGlobalPortTemplateRule> | ref<SchematicIfFrameBorderTemplateRule> | ref<SchematicInterconnectTerminatorTemplateRule> | ref<SchematicJunctionTemplateRule> | ref<SchematicMasterPortTemplateRule> | ref<SchematicOffPageConnectorTemplateRule> | ref<SchematicOnPageConnectorTemplateRule> | ref<SchematicOtherwiseFrameBorderTemplateRule> | ref<SchematicRipperTemplateRule> | ref<SchematicSymbolBorderTemplateRule> | ref<SchematicSymbolPortTemplateRule> | ref<UserDataRule>)));
  EDIF_FORM(FahrenheitRule, "(fahrenheit",
           arg(unitExponentExpr));
  EDIF_FORM0(EfalseRule, "(false");
  EDIF_FORM(FaradRule, "(farad",
           arg(unitExponentExpr));
  EDIF_FORM(FigureRule, "(figure",
           arg(figureGroupNameRefExpr | ref<FigureGroupOverrideRule>) +
           many(arg(ref<CircleRule> | ref<CommentRule> | ref<ComplexGeometryRule> | ref<DotRule> | ref<OpenShapeRule> | ref<PathRule> | ref<PolygonRule> | ref<RectangleRule> | ref<ShapeRule> | ref<UserDataRule>)));
  EDIF_FORM(FigureGroupRule, "(figureGroup",
           arg(figureGroupNameDefExpr) +
           many(arg(ref<CommentRule> | ref<CornerTypeRule> | ref<DisplayAttributesRule> | ref<EndTypeRule> | ref<NameInformationRule> | ref<PathWidthRule> | ref<PropertyRule> | ref<UserDataRule>)));
  EDIF_FORM(FigureGroupNameCaseSensitiveRule, "(figureGroupNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(FigureGroupOverrideRule, "(figureGroupOverride",
           arg(figureGroupNameRefExpr) +
           many(arg(ref<CommentRule> | ref<CornerTypeRule> | ref<DisplayAttributesRule> | ref<EndTypeRule> | ref<PathWidthRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(FillPatternRule, "(fillPattern",
           arg(ref<PixelPatternRule>));
  EDIF_FORM(FillPatternVisibleRule, "(fillPatternVisible",
           arg(booleanValueExpr));
  EDIF_FORM0(FixedRule, "(fixed");
  EDIF_FORM(FontRule, "(font",
           arg(fontNameDefExpr) +
           arg(ref<TypefaceRule>) +
           arg(ref<FontProportionsRule>) +
           many(arg(ref<BoldStyleRule> | ref<ItalicStyleRule> | ref<PropertyRule> | ref<ProportionalFontRule> | ref<UserDataRule>)));
  EDIF_FORM(FontCapitalHeightRule, "(fontCapitalHeight",
           arg(lengthValueExpr));
  EDIF_FORM(FontDefinitionsRule, "(fontDefinitions",
           option(arg(ref<FontsRule>)));
  EDIF_FORM(FontDescentRule, "(fontDescent",
           arg(lengthValueExpr));
  EDIF_FORM(FontHeightRule, "(fontHeight",
           arg(lengthValueExpr));
  EDIF_FORM(FontProportionsRule, "(fontProportions",
           arg(ref<FontHeightRule>) +
           arg(ref<FontDescentRule>) +
           arg(ref<FontCapitalHeightRule>) +
           arg(ref<FontWidthRule>));
  EDIF_FORM(FontRefRule, "(fontRef",
           arg(fontNameRefExpr));
  EDIF_FORM(FontsRule, "(fonts",
           arg(ref<SetDistanceRule>) +
           many(arg(ref<FontRule>)));
  EDIF_FORM(FontWidthRule, "(fontWidth",
           arg(lengthValueExpr));
  EDIF_FORM(ForbiddenEventRule, "(forbiddenEvent",
           arg(ref<TimeIntervalRule>) +
           many(arg(ref<EventRule>)));
  EDIF_FORM(ForFrameRule, "(forFrame",
           arg(frameNameDefExpr) +
           arg(ref<RepetitionCountRule>) +
           arg(ref<ForFrameIndexRule>) +
           arg(ref<LogicalConnectivityRule>) +
           many(arg(ref<CommentRule> | ref<DocumentationRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<UserDataRule>)));
  EDIF_FORM(ForFrameAnnotateRule, "(forFrameAnnotate",
           arg(extendForFrameMemberDefExpr) +
           many(arg(ref<CommentRule> | ref<ForFrameAnnotateRule> | ref<IfFrameAnnotateRule> | ref<InterconnectAnnotateRule> | ref<LeafOccurrenceAnnotateRule> | ref<OccurrenceAnnotateRule> | ref<OtherwiseFrameAnnotateRule>)));
  EDIF_FORM(ForFrameIndexRule, "(forFrameIndex",
           arg(indexNameDefExpr) +
           many(arg(ref<IndexStartRule> | ref<IndexStepRule> | ref<NameInformationRule>)));
  EDIF_FORM(ForFrameIndexDisplayRule, "(forFrameIndexDisplay",
           many(arg(ref<IndexEndDisplayRule> | ref<IndexNameDisplayRule> | ref<IndexStartDisplayRule> | ref<IndexStepDisplayRule>)));
  EDIF_FORM(ForFrameIndexNameCaseSensitiveRule, "(forFrameIndexNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(ForFrameIndexRefRule, "(forFrameIndexRef",
           arg(indexNameRefExpr));
  EDIF_FORM(ForFrameMemberRefRule, "(forFrameMemberRef",
           arg(frameNameRefExpr) +
           arg(ref<IndexValueRule>));
  EDIF_FORM(ForFrameRefRule, "(forFrameRef",
           arg(frameNameRefExpr));
  EDIF_FORM(FrameConfigurationRule, "(frameConfiguration",
           arg(frameNameRefExpr) +
           many(arg(ref<FrameConfigurationRule> | ref<InstanceConfigurationRule>)));
  EDIF_FORM(FrameNameCaseSensitiveRule, "(frameNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(FrameRefRule, "(frameRef",
           arg(frameNameRefExpr));
  EDIF_FORM0(FromBottomRule, "(fromBottom");
  EDIF_FORM0(FromLeftRule, "(fromLeft");
  EDIF_FORM0(FromRightRule, "(fromRight");
  EDIF_FORM0(FromTopRule, "(fromTop");
  EDIF_FORM0(GeneratedRule, "(generated");
  EDIF_FORM(GeometryMacroRule, "(geometryMacro",
           arg(libraryObjectNameDefExpr) +
           arg(ref<GeometryMacroHeaderRule>) +
           many(arg(ref<CircleRule> | ref<CommentRule> | ref<ComplexGeometryRule> | ref<DotRule> | ref<OpenShapeRule> | ref<PathRule> | ref<PolygonRule> | ref<RectangleRule> | ref<ShapeRule> | ref<UserDataRule>)));
  EDIF_FORM(GeometryMacroHeaderRule, "(geometryMacroHeader",
           arg(ref<GeometryMacroUnitsRule>) +
           many(arg(ref<BackgroundColorRule> | ref<DocumentationRule> | ref<NameInformationRule> | ref<OriginalBoundingBoxRule> | ref<PropertyRule> | ref<StatusRule>)));
  EDIF_FORM(GeometryMacroRefRule, "(geometryMacroRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(GeometryMacroUnitsRule, "(geometryMacroUnits",
           option(arg(ref<SetAngleRule>)));
  EDIF_FORM(GlobalPortRule, "(globalPort",
           arg(globalPortNameDefExpr) +
           many(arg(ref<CommentRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<SchematicGlobalPortAttributesRule> | ref<UserDataRule>)));
  EDIF_FORM(GlobalPortBundleRule, "(globalPortBundle",
           arg(globalPortNameDefExpr) +
           arg(ref<GlobalPortListRule>) +
           many(arg(ref<CommentRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<UserDataRule>)));
  EDIF_FORM(GlobalPortDefinitionsRule, "(globalPortDefinitions",
           many(arg(ref<GlobalPortRule> | ref<GlobalPortBundleRule>)));
  EDIF_FORM(GlobalPortListRule, "(globalPortList",
           many(arg(ref<GlobalPortRefRule>)));
  EDIF_FORM(GlobalPortNameCaseSensitiveRule, "(globalPortNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(GlobalPortNameDisplayRule, "(globalPortNameDisplay",
           many(arg(ref<DisplayRule> | ref<DisplayNameOverrideRule>)));
  EDIF_FORM(GlobalPortPropertyDisplayRule, "(globalPortPropertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<DisplayRule> | ref<PropertyNameDisplayRule>)));
  EDIF_FORM(GlobalPortRefRule, "(globalPortRef",
           arg(globalPortNameRefExpr));
  EDIF_FORM(GlobalPortScopeRule, "(globalPortScope",
           arg(globalPortNameRefExpr));
  EDIF_FORM(HenryRule, "(henry",
           arg(unitExponentExpr));
  EDIF_FORM(HertzRule, "(hertz",
           arg(unitExponentExpr));
  EDIF_FORM(HorizontalJustificationRule, "(horizontalJustification",
           arg(ref<LeftJustifyRule> | ref<CenterJustifyRule> | ref<RightJustifyRule>));
  EDIF_FORM(HotspotRule, "(hotspot",
           arg(pointValueExpr) +
           many(arg(ref<HotspotConnectDirectionRule> | ref<HotspotNameDisplayRule> | ref<NameInformationRule> | ref<SchematicWireAffinityRule>)));
  EDIF_FORM(HotspotConnectDirectionRule, "(hotspotConnectDirection",
           many(arg(ref<FromBottomRule> | ref<FromLeftRule> | ref<FromRightRule> | ref<FromTopRule>)));
  EDIF_FORM(HotspotGridRule, "(hotspotGrid",
           arg(lengthValueExpr));
  EDIF_FORM(HotspotNameCaseSensitiveRule, "(hotspotNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(HotspotNameDisplayRule, "(hotspotNameDisplay",
           many(arg(ref<DisplayRule> | ref<DisplayNameOverrideRule>)));
  EDIF_FORM(IeeeSectionRule, "(ieeeSection",
           many(arg(integerTokenExpr)));
  EDIF_FORM(IeeeStandardRule, "(ieeeStandard",
           arg(ieeeDesignationExpr) +
           arg(ref<YearRule>) +
           many(arg(ref<CommentRule> | ref<IeeeSectionRule>)));
  EDIF_FORM(IfFrameRule, "(ifFrame",
           arg(frameNameDefExpr) +
           arg(ref<ConditionRule>) +
           arg(ref<LogicalConnectivityRule>) +
           many(arg(ref<CommentRule> | ref<NameInformationRule> | ref<DocumentationRule> | ref<PropertyRule> | ref<UserDataRule>)));
  EDIF_FORM(IfFrameAnnotateRule, "(ifFrameAnnotate",
           arg(extendFrameDefExpr) +
           many(arg(ref<CommentRule> | ref<ForFrameAnnotateRule> | ref<IfFrameAnnotateRule> | ref<InterconnectAnnotateRule> | ref<LeafOccurrenceAnnotateRule> | ref<OccurrenceAnnotateRule> | ref<OtherwiseFrameAnnotateRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(IfFrameRefRule, "(ifFrameRef",
           arg(frameNameRefExpr));
  EDIF_FORM(IfFrameSetRule, "(ifFrameSet",
           many(arg(ref<IfFrameRefRule>)));
  EDIF_FORM0(IgnoreRule, "(ignore");
  EDIF_FORM(ImplementationNameCaseSensitiveRule, "(implementationNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(ImplementationNameDisplayRule, "(implementationNameDisplay",
           many(arg(ref<DisplayRule> | ref<DisplayNameOverrideRule>)));
  EDIF_FORM(IndexEndDisplayRule, "(indexEndDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(IndexNameDisplayRule, "(indexNameDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(IndexStartRule, "(indexStart",
           arg(integerExpressionExpr));
  EDIF_FORM(IndexStartDisplayRule, "(indexStartDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(IndexStepRule, "(indexStep",
           arg(integerExpressionExpr));
  EDIF_FORM(IndexStepDisplayRule, "(indexStepDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(IndexValueRule, "(indexValue",
           arg(integerTokenExpr));
  EDIF_FORM0(InputRule, "(input");
  EDIF_FORM(InputPortRule, "(inputPort",
           option(arg(ref<InputPortAttributesRule>)));
  EDIF_FORM(InputPortAttributesRule, "(inputPortAttributes",
           many(arg(ref<DcFanOutLoadRule> | ref<DcMaxFanInRule>)));
  EDIF_FORM(InstanceRule, "(instance",
           arg(instanceNameDefExpr) +
           arg(ref<ClusterRefRule>) +
           many(arg(ref<BooleanParameterAssignRule> | ref<CellPropertyOverrideRule> | ref<ClusterPropertyOverrideRule> | ref<CommentRule> | ref<DesignatorRule> | ref<InstanceNameGeneratorRule> | ref<InstancePortAttributesRule> | ref<InstanceWidthRule> | ref<IntegerParameterAssignRule> | ref<NameInformationRule> | ref<NumberParameterAssignRule> | ref<PropertyRule> | ref<StringParameterAssignRule> | ref<TimingRule> | ref<UserDataRule>)));
  EDIF_FORM(InstanceConfigurationRule, "(instanceConfiguration",
           arg(instanceNameRefExpr) +
           arg(ref<ClusterConfigurationRefRule>));
  EDIF_FORM0(InstanceIndexValueRule, "(instanceIndexValue");
  EDIF_FORM(InstanceMemberRefRule, "(instanceMemberRef",
           arg(instanceNameRefExpr) +
           arg(ref<IndexValueRule>));
  EDIF_FORM(InstanceNameCaseSensitiveRule, "(instanceNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(InstanceNameDisplayRule, "(instanceNameDisplay",
           many(arg(ref<DisplayRule> | ref<DisplayNameOverrideRule>)));
  EDIF_FORM(InstanceNameGeneratorRule, "(instanceNameGenerator",
           arg(stringExpressionExpr));
  EDIF_FORM(InstanceNameGeneratorDisplayRule, "(instanceNameGeneratorDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(InstancePortAttributeDisplayRule, "(instancePortAttributeDisplay",
           arg(symbolPortImplementationNameRefExpr) +
           arg(ref<PortRefRule>) +
           many(arg(ref<PortPropertyDisplayOverrideRule> | ref<PortAttributeDisplayRule>)));
  EDIF_FORM(InstancePortAttributesRule, "(instancePortAttributes",
           arg(extendPortDefExpr) +
           many(arg(ref<AcLoadRule> | ref<CommentRule> | ref<ConnectedSignalIndexGeneratorRule> | ref<DesignatorRule> | ref<DirectionalPortAttributeOverrideRule> | ref<PortDelayRule> | ref<PortDelayOverrideRule> | ref<PortLoadDelayRule> | ref<PortLoadDelayOverrideRule> | ref<PortPropertyOverrideRule> | ref<PropertyRule> | ref<UnusedRule>)));
  EDIF_FORM(InstancePropertyDisplayRule, "(instancePropertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<DisplayRule>)) +
           option(arg(ref<PropertyNameDisplayRule>) +
                  many(arg(ref<DisplayRule>))));
  EDIF_FORM(InstancePropertyOverrideRule, "(instancePropertyOverride",
           arg(propertyNameRefExpr) +
           arg(typedValueExpr | ref<UntypedRule>) +
           many(arg(ref<CommentRule> | ref<FixedRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(InstanceRefRule, "(instanceRef",
           arg(instanceNameRefExpr));
  EDIF_FORM(InstanceWidthRule, "(instanceWidth",
           arg(integerExpressionExpr));
  EDIF_FORM(InstanceWidthDisplayRule, "(instanceWidthDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(IntegerRule, "(integer",
           arg(integerExpressionExpr));
  EDIF_FORM(IntegerConstantRule, "(integerConstant",
           arg(constantNameDefExpr) +
           arg(integerTokenExpr));
  EDIF_FORM(IntegerConstantRefRule, "(integerConstantRef",
           arg(constantNameRefExpr));
  EDIF_FORM(IntegerEqualRule, "(integerEqual",
           arg(firstIntegerExpressionExpr) +
           arg(secondIntegerExpressionExpr));
  EDIF_FORM(IntegerParameterRule, "(integerParameter",
           arg(parameterNameDefExpr) +
           many(arg(ref<IntegerRule> | ref<NameInformationRule>)));
  EDIF_FORM(IntegerParameterAssignRule, "(integerParameterAssign",
           arg(parameterNameRefExpr) +
           arg(integerExpressionExpr));
  EDIF_FORM(IntegerParameterRefRule, "(integerParameterRef",
           arg(parameterNameRefExpr));
  EDIF_FORM(IntegerProductRule, "(integerProduct",
           many(arg(integerExpressionExpr)));
  EDIF_FORM(IntegerQuotientRule, "(integerQuotient",
           arg(dividendExpr) +
           arg(divisorExpr));
  EDIF_FORM(IntegerRemainderRule, "(integerRemainder",
           arg(dividendExpr) +
           arg(divisorExpr));
  EDIF_FORM(IntegerSubtractRule, "(integerSubtract",
           arg(minuendExpr) +
           arg(subtrahendExpr));
  EDIF_FORM(IntegerSumRule, "(integerSum",
           many(arg(integerExpressionExpr)));
  EDIF_FORM(InterconnectAnnotateRule, "(interconnectAnnotate",
           arg(extendInterconnectDefExpr) +
           many(arg(ref<CommentRule> | ref<InterconnectAnnotateRule> | ref<CriticalityRule> | ref<InterconnectDelayRule> | ref<PropertyRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(InterconnectAttachedTextRule, "(interconnectAttachedText",
           arg(attachmentPointExpr) +
           many(arg(ref<AnnotateRule> | ref<ConnectivityTagGeneratorDisplayRule> | ref<CriticalityDisplayRule> | ref<InterconnectDelayDisplayRule> | ref<InterconnectNameDisplayRule> | ref<InterconnectPropertyDisplayRule>)));
  EDIF_FORM(InterconnectDelayRule, "(interconnectDelay",
           arg(interconnectDelayNameDefExpr) +
           arg(derivationExpr) +
           arg(ref<DelayRule>) +
           many(arg(ref<BecomesRule> | ref<TransitionRule>)));
  EDIF_FORM(InterconnectDelayDisplayRule, "(interconnectDelayDisplay",
           arg(interconnectDelayNameRefExpr) +
           many(arg(ref<DisplayRule>)));
  EDIF_FORM(InterconnectHeaderRule, "(interconnectHeader",
           many(arg(ref<CriticalityRule> | ref<DocumentationRule> | ref<InterconnectDelayRule> | ref<NameInformationRule> | ref<PropertyRule>)));
  EDIF_FORM(InterconnectNameCaseSensitiveRule, "(interconnectNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(InterconnectNameDisplayRule, "(interconnectNameDisplay",
           many(arg(ref<DisplayRule> | ref<DisplayNameOverrideRule>)));
  EDIF_FORM(InterconnectPropertyDisplayRule, "(interconnectPropertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<DisplayRule>)) +
           option(arg(ref<PropertyNameDisplayRule>) +
                  many(arg(ref<DisplayRule>))));
  EDIF_FORM(InterconnectRefRule, "(interconnectRef",
           arg(interconnectNameRefExpr) +
           option(arg(ref<PageRefRule>)));
  EDIF_FORM(InterconnectSetRule, "(interconnectSet",
           many(arg(ref<InterconnectRefRule>)));
  EDIF_FORM(EinterfaceRule, "(interface",
           arg(ref<InterfaceUnitsRule>) +
           many(arg(ref<DesignatorRule> | ref<BooleanParameterRule> | ref<IntegerParameterRule> | ref<InterfaceJoinedRule> | ref<MustJoinRule> | ref<NumberParameterRule> | ref<PermutableRule> | ref<PortRule> | ref<PortBundleRule> | ref<StringParameterRule> | ref<TimingRule> | ref<WeakJoinedRule>)));
  EDIF_FORM(InterfaceJoinedRule, "(interfaceJoined",
           many(arg(ref<PortRefRule>)));
  EDIF_FORM(InterfaceUnitsRule, "(interfaceUnits",
           many(arg(ref<SetCapacitanceRule> | ref<SetTimeRule>)));
  EDIF_FORM(Iso8859Rule, "(iso8859",
           arg(iso8859PartExpr));
  EDIF_FORM0(IsolatedRule, "(isolated");
  EDIF_FORM0(ItalicStyleRule, "(italicStyle");
  EDIF_FORM0(Jisx0201Rule, "(jisx0201");
  EDIF_FORM0(Jisx0208Rule, "(jisx0208");
  EDIF_FORM0(JoinedAsSignalRule, "(joinedAsSignal");
  EDIF_FORM(JouleRule, "(joule",
           arg(unitExponentExpr));
  EDIF_FORM0(K0KeywordLevelRule, "(k0KeywordLevel");
  EDIF_FORM(K1KeywordAliasRule, "(k1KeywordAlias",
           arg(k1KeywordNameDefExpr) +
           arg(k1KeywordNameRefExpr));
  EDIF_FORM(K1KeywordLevelRule, "(k1KeywordLevel",
           many(arg(ref<K1KeywordAliasRule>)));
  EDIF_FORM(K2ActualRule, "(k2Actual",
           arg(k2FormalNameRefExpr));
  EDIF_FORM(K2BuildRule, "(k2Build",
           arg(k1KeywordNameRefExpr) +
           many(arg(ref<CommentRule> | ref<K2ActualRule> | ref<K2BuildRule> | ref<K2LiteralRule>)));
  EDIF_FORM(K2FormalRule, "(k2Formal",
           arg(k2FormalNameDefExpr) +
           arg(ref<K2OptionalRule> | ref<K2RequiredRule>));
  EDIF_FORM(K2GenerateRule, "(k2Generate",
           many(arg(ref<CommentRule> | ref<K2ActualRule> | ref<K2BuildRule> | ref<K2LiteralRule>)));
  EDIF_FORM(K2KeywordDefineRule, "(k2KeywordDefine",
           arg(k1KeywordNameDefExpr) +
           arg(ref<K2KeywordParametersRule>) +
           arg(ref<K2GenerateRule>));
  EDIF_FORM(K2KeywordLevelRule, "(k2KeywordLevel",
           many(arg(ref<K1KeywordAliasRule> | ref<K2KeywordDefineRule>)));
  EDIF_FORM(K2KeywordParametersRule, "(k2KeywordParameters",
           many(arg(ref<K2FormalRule>)));
  EDIF_FORM(K2LiteralRule, "(k2Literal",
           many(arg(ref<IDENTIFIER> | integerTokenExpr | stringTokenExpr)));
  EDIF_FORM(K2OptionalRule, "(k2Optional",
           arg(ref<K2LiteralRule> | ref<K2ActualRule> | ref<K2BuildRule>));
  EDIF_FORM0(K2RequiredRule, "(k2Required");
  EDIF_FORM(K3BuildRule, "(k3Build",
           arg(k1KeywordNameRefExpr) +
           many(arg(ref<CommentRule> | ref<K2ActualRule> | ref<K2LiteralRule> | ref<K3BuildRule> | ref<K3ForEachRule>)));
  EDIF_FORM0(K3CollectorRule, "(k3Collector");
  EDIF_FORM(K3ForEachRule, "(k3ForEach",
           arg(k2FormalNameRefExpr | ref<K3FormalListRule>) +
           many(arg(ref<CommentRule> | ref<K2ActualRule> | ref<K2LiteralRule> | ref<K3BuildRule> | ref<K3ForEachRule>)));
  EDIF_FORM(K3FormalRule, "(k3Formal",
           arg(k2FormalNameDefExpr) +
           arg(ref<K2OptionalRule> | ref<K2RequiredRule> | ref<K3CollectorRule>));
  EDIF_FORM(K3FormalListRule, "(k3FormalList",
           many(arg(k2FormalNameRefExpr)));
  EDIF_FORM(K3GenerateRule, "(k3Generate",
           many(arg(ref<CommentRule> | ref<K2ActualRule> | ref<K2BuildRule> | ref<K2LiteralRule> | ref<K3ForEachRule>)));
  EDIF_FORM(K3KeywordDefineRule, "(k3KeywordDefine",
           arg(k1KeywordNameDefExpr) +
           arg(ref<K3KeywordParametersRule>) +
           arg(ref<K3GenerateRule>));
  EDIF_FORM(K3KeywordLevelRule, "(k3KeywordLevel",
           many(arg(ref<K1KeywordAliasRule> | ref<K3KeywordDefineRule>)));
  EDIF_FORM(K3KeywordParametersRule, "(k3KeywordParameters",
           many(arg(ref<K3FormalRule>)));
  EDIF_FORM(KelvinRule, "(kelvin",
           arg(unitExponentExpr));
  EDIF_FORM(KeywordMapRule, "(keywordMap",
           arg(ref<K0KeywordLevelRule> | ref<K1KeywordLevelRule> | ref<K2KeywordLevelRule> | ref<K3KeywordLevelRule>) +
           many(arg(ref<CommentRule>)));
  EDIF_FORM(KilogramRule, "(kilogram",
           arg(unitExponentExpr));
  EDIF_FORM0(LeafRule, "(leaf");
  EDIF_FORM(LeafOccurrenceAnnotateRule, "(leafOccurrenceAnnotate",
           arg(extendInstanceDefExpr | extendInstanceMemberDefExpr) +
           many(arg(ref<CellPropertyOverrideRule> | ref<ClusterPropertyOverrideRule> | ref<CommentRule> | ref<DesignatorRule> | ref<InstancePropertyOverrideRule> | ref<PortAnnotateRule> | ref<PropertyRule>)));
  EDIF_FORM0(LeftJustifyRule, "(leftJustify");
  EDIF_FORM(LessThanRule, "(lessThan",
           many(arg(integerExpressionExpr)));
  EDIF_FORM(LessThanOrEqualRule, "(lessThanOrEqual",
           many(arg(integerExpressionExpr)));
  EDIF_FORM(LibraryRule, "(library",
           arg(libraryNameDefExpr) +
           arg(ref<LibraryHeaderRule>) +
           many(arg(ref<CellRule> | ref<SchematicInterconnectTerminatorTemplateRule> | ref<SchematicJunctionTemplateRule> | ref<SchematicGlobalPortTemplateRule> | ref<SchematicMasterPortTemplateRule> | ref<SchematicOffPageConnectorTemplateRule> | ref<SchematicOnPageConnectorTemplateRule> | ref<SchematicRipperTemplateRule> | ref<SchematicSymbolBorderTemplateRule> | ref<SchematicSymbolPortTemplateRule> | ref<PageBorderTemplateRule> | ref<PageTitleBlockTemplateRule> | ref<CommentRule> | ref<GeometryMacroRule> | ref<SchematicFigureMacroRule> | ref<SchematicForFrameBorderTemplateRule> | ref<SchematicIfFrameBorderTemplateRule> | ref<SchematicOtherwiseFrameBorderTemplateRule> | ref<UserDataRule>)));
  EDIF_FORM(LibraryHeaderRule, "(libraryHeader",
           arg(ref<EdifLevelRule>) +
           arg(ref<NameCaseSensitivityRule>) +
           arg(ref<TechnologyRule>) +
           many(arg(ref<BackgroundColorRule> | ref<DocumentationRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<StatusRule>)));
  EDIF_FORM(LibraryNameCaseSensitiveRule, "(libraryNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(LibraryObjectNameCaseSensitiveRule, "(libraryObjectNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(LibraryRefRule, "(libraryRef",
           arg(libraryNameRefExpr));
  EDIF_FORM(LoadDelayRule, "(loadDelay",
           arg(acLoadFactorTimeExpr) +
           arg(acLoadFactorCapacitanceExpr));
  EDIF_FORM(LocalPortGroupRule, "(localPortGroup",
           arg(localPortGroupNameDefExpr) +
           arg(ref<PortListRule>) +
           many(arg(ref<CommentRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<UserDataRule>)));
  EDIF_FORM(LocalPortGroupNameCaseSensitiveRule, "(localPortGroupNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(LocalPortGroupRefRule, "(localPortGroupRef",
           arg(localPortGroupNameRefExpr));
  EDIF_FORM(LogicalConnectivityRule, "(logicalConnectivity",
           many(arg(ref<CommentRule> | ref<ForFrameRule> | ref<IfFrameRule> | ref<InstanceRule> | ref<OtherwiseFrameRule> | ref<SignalRule> | ref<SignalGroupRule> | ref<UserDataRule>)));
  EDIF_FORM(LogicDefinitionsRule, "(logicDefinitions",
           arg(ref<SetVoltageRule>) +
           arg(ref<SetCurrentRule>) +
           many(arg(ref<CommentRule> | ref<LogicValueRule>)));
  EDIF_FORM(LogicListRule, "(logicList",
           many(arg(ref<IgnoreRule> | logicNameRefExpr | ref<LogicOneOfRule>)));
  EDIF_FORM(LogicMapInputRule, "(logicMapInput",
           many(arg(ref<LogicRefRule>)));
  EDIF_FORM(LogicMapOutputRule, "(logicMapOutput",
           many(arg(ref<LogicRefRule>)));
  EDIF_FORM(LogicModelUnitsRule, "(logicModelUnits",
           many(arg(ref<SetCapacitanceRule> | ref<SetTimeRule>)));
  EDIF_FORM(LogicModelViewRule, "(logicModelView",
           arg(viewNameDefExpr) +
           many(arg(ref<CommentRule> | ref<NameInformationRule> | ref<UserDataRule>)));
  EDIF_FORM(LogicOneOfRule, "(logicOneOf",
           many(arg(ref<LogicListRule> | logicNameRefExpr)));
  EDIF_FORM(LogicRefRule, "(logicRef",
           arg(logicNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(LogicValueRule, "(logicValue",
           arg(logicNameDefExpr) +
           many(arg(ref<BooleanMapRule> | ref<CommentRule> | ref<CompoundRule> | ref<CurrentMapRule> | ref<DominatesRule> | ref<IsolatedRule> | ref<LogicMapInputRule> | ref<LogicMapOutputRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<ResolvesRule> | ref<StrongRule> | ref<VoltageMapRule> | ref<WeakRule>)));
  EDIF_FORM0(LsbToMsbRule, "(lsbToMsb");
  EDIF_FORM(MaskLayoutUnitsRule, "(maskLayoutUnits",
           many(arg(ref<SetAngleRule> | ref<SetCapacitanceRule> | ref<SetDistanceRule> | ref<SetTimeRule>)));
  EDIF_FORM(MaskLayoutViewRule, "(maskLayoutView",
           arg(viewNameDefExpr) +
           many(arg(ref<CommentRule> | ref<NameInformationRule> | ref<UserDataRule>)));
  EDIF_FORM0(MeasuredRule, "(measured");
  EDIF_FORM(MeterRule, "(meter",
           arg(unitExponentExpr));
  EDIF_FORM0(MiddleJustifyRule, "(middleJustify");
  EDIF_FORM0(MinimalWidthRule, "(minimalWidth");
  EDIF_FORM(MinimumStringLengthRule, "(minimumStringLength",
           arg(substringLengthExpr));
  EDIF_FORM(MiNoMaxRule, "(miNoMax",
           arg(miNoMaxValueExpr));
  EDIF_FORM0(MixedDirectionRule, "(mixedDirection");
  EDIF_FORM(MnmRule, "(mnm",
           arg(numberValueExpr | ref<UndefinedRule> | ref<UnconstrainedRule>) +
           arg(numberValueExpr | ref<UndefinedRule> | ref<UnconstrainedRule>) +
           arg(numberValueExpr | ref<UndefinedRule> | ref<UnconstrainedRule>));
  EDIF_FORM(MoleRule, "(mole",
           arg(unitExponentExpr));
  EDIF_FORM0(MsbToLsbRule, "(msbToLsb");
  EDIF_FORM(MustJoinRule, "(mustJoin",
           many(arg(ref<InterfaceJoinedRule> | ref<PortRefRule> | ref<WeakJoinedRule>)));
  EDIF_FORM(NameAliasRule, "(nameAlias",
           arg(originalNameExpr) +
           many(arg(ref<DisplayNameRule> | ref<GeneratedRule> | ref<NameStructureRule>)));
  EDIF_FORM(NameCaseSensitivityRule, "(nameCaseSensitivity",
           many(arg(ref<ClusterConfigurationNameCaseSensitiveRule> | ref<ClusterNameCaseSensitiveRule> | ref<DesignHierarchyNameCaseSensitiveRule> | ref<DesignNameCaseSensitiveRule> | ref<DocumentationNameCaseSensitiveRule> | ref<FigureGroupNameCaseSensitiveRule> | ref<ForFrameIndexNameCaseSensitiveRule> | ref<FrameNameCaseSensitiveRule> | ref<GlobalPortNameCaseSensitiveRule> | ref<HotspotNameCaseSensitiveRule> | ref<ImplementationNameCaseSensitiveRule> | ref<InstanceNameCaseSensitiveRule> | ref<InterconnectNameCaseSensitiveRule> | ref<LibraryNameCaseSensitiveRule> | ref<LibraryObjectNameCaseSensitiveRule> | ref<LocalPortGroupNameCaseSensitiveRule> | ref<PageNameCaseSensitiveRule> | ref<ParameterNameCaseSensitiveRule> | ref<PortNameCaseSensitiveRule> | ref<PropertyNameCaseSensitiveRule> | ref<SignalGroupNameCaseSensitiveRule> | ref<SignalNameCaseSensitiveRule> | ref<ViewGroupNameCaseSensitiveRule> | ref<ViewNameCaseSensitiveRule>)));
  EDIF_FORM(NameDimensionRule, "(nameDimension",
           arg(ref<NameDimensionStructureRule>) +
           option(arg(ref<BitOrderRule>)));
  EDIF_FORM(NameDimensionStructureRule, "(nameDimensionStructure",
           many(arg(ref<ComplementedNameRule> | ref<ComplexNameRule> | integerValueExpr | ref<SequenceRule> | simpleNameExpr)));
  EDIF_FORM(NameInformationRule, "(nameInformation",
           arg(ref<PrimaryNameRule>) +
           many(arg(ref<NameAliasRule>)));
  EDIF_FORM(NamePartSeparatorRule, "(namePartSeparator",
           arg(stringTokenExpr));
  EDIF_FORM(NameStructureRule, "(nameStructure",
           arg(simpleNameExpr | ref<ComplexNameRule> | ref<ComplementedNameRule>));
  EDIF_FORM0(NarrowPortRule, "(narrowPort");
  EDIF_FORM0(NarrowWireRule, "(narrowWire");
  EDIF_FORM0(NoHotspotGridRule, "(noHotspotGrid");
  EDIF_FORM(NominalHotspotGridRule, "(nominalHotspotGrid",
           arg(lengthValueExpr));
  EDIF_FORM(NonPermutableRule, "(nonPermutable",
           many(arg(ref<PermutableRule> | ref<PortRefRule>)));
  EDIF_FORM(NotRule, "(not",
           arg(booleanExpressionExpr));
  EDIF_FORM0(NotInheritedRule, "(notInherited");
  EDIF_FORM(NumberRule, "(number",
           arg(numberExpressionExpr));
  EDIF_FORM(NumberParameterRule, "(numberParameter",
           arg(parameterNameDefExpr) +
           many(arg(ref<NameInformationRule> | ref<NumberRule>)));
  EDIF_FORM(NumberParameterAssignRule, "(numberParameterAssign",
           arg(parameterNameRefExpr) +
           arg(numberExpressionExpr));
  EDIF_FORM(NumberParameterRefRule, "(numberParameterRef",
           arg(parameterNameRefExpr));
  EDIF_FORM(NumberPointRule, "(numberPoint",
           arg(xNumberValueExpr) +
           arg(yNumberValueExpr));
  EDIF_FORM(OccurrenceAnnotateRule, "(occurrenceAnnotate",
           arg(extendInstanceDefExpr | extendInstanceMemberDefExpr) +
           many(arg(ref<CellPropertyOverrideRule> | ref<ClusterPropertyOverrideRule> | ref<CommentRule> | ref<DesignatorRule> | ref<ForFrameAnnotateRule> | ref<IfFrameAnnotateRule> | ref<InstancePropertyOverrideRule> | ref<InterconnectAnnotateRule> | ref<LeafOccurrenceAnnotateRule> | ref<OccurrenceAnnotateRule> | ref<OtherwiseFrameAnnotateRule> | ref<PageAnnotateRule> | ref<PortAnnotateRule> | ref<PropertyRule> | ref<SignalAnnotateRule> | ref<SignalGroupAnnotateRule> | ref<TimingRule> | ref<ViewPropertyOverrideRule>)));
  EDIF_FORM(OccurrenceHierarchyAnnotateRule, "(occurrenceHierarchyAnnotate",
           many(arg(ref<CellPropertyOverrideRule> | ref<ClusterPropertyOverrideRule> | ref<CommentRule> | ref<ForFrameAnnotateRule> | ref<IfFrameAnnotateRule> | ref<InterconnectAnnotateRule> | ref<LeafOccurrenceAnnotateRule> | ref<OccurrenceAnnotateRule> | ref<OtherwiseFrameAnnotateRule> | ref<PageAnnotateRule> | ref<PortAnnotateRule> | ref<SignalAnnotateRule> | ref<SignalGroupAnnotateRule> | ref<TimingRule> | ref<ViewPropertyOverrideRule>)));
  EDIF_FORM(OffsetEventRule, "(offsetEvent",
           arg(ref<EventRule>) +
           arg(numberValueExpr));
  EDIF_FORM(OhmRule, "(ohm",
           arg(unitExponentExpr));
  EDIF_FORM(OpenShapeRule, "(openShape",
           arg(ref<CurveRule>));
  EDIF_FORM(OrRule, "(or",
           many(arg(booleanExpressionExpr)));
  EDIF_FORM(OriginRule, "(origin",
           arg(pointValueExpr));
  EDIF_FORM(OriginalBoundingBoxRule, "(originalBoundingBox",
           arg(ref<RectangleRule>));
  EDIF_FORM(OriginalDrawingDateRule, "(originalDrawingDate",
           arg(ref<DateRule>));
  EDIF_FORM(OriginalDrawingDateDisplayRule, "(originalDrawingDateDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(OtherwiseFrameRule, "(otherwiseFrame",
           arg(frameNameDefExpr) +
           arg(ref<IfFrameSetRule>) +
           arg(ref<LogicalConnectivityRule>) +
           many(arg(ref<CommentRule> | ref<DocumentationRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<UserDataRule>)));
  EDIF_FORM(OtherwiseFrameAnnotateRule, "(otherwiseFrameAnnotate",
           arg(extendFrameDefExpr) +
           many(arg(ref<CommentRule> | ref<ForFrameAnnotateRule> | ref<IfFrameAnnotateRule> | ref<InterconnectAnnotateRule> | ref<LeafOccurrenceAnnotateRule> | ref<OccurrenceAnnotateRule> | ref<OtherwiseFrameAnnotateRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(OtherwiseFrameRefRule, "(otherwiseFrameRef",
           arg(frameNameRefExpr));
  EDIF_FORM0(OutputRule, "(output");
  EDIF_FORM(OutputPortRule, "(outputPort",
           option(arg(ref<OutputPortAttributesRule>)));
  EDIF_FORM(OutputPortAttributesRule, "(outputPortAttributes",
           many(arg(ref<DcFanInLoadRule> | ref<DcMaxFanOutRule>)));
  EDIF_FORM(OwnerRule, "(owner",
           arg(stringValueExpr));
  EDIF_FORM(PageRule, "(page",
           arg(pageNameDefExpr) +
           arg(ref<PageHeaderRule>) +
           many(arg(ref<CellPropertyDisplayRule> | ref<ClusterPropertyDisplayRule> | ref<CommentRule> | ref<LocalPortGroupRule> | ref<PageCommentGraphicsRule> | ref<PageTitleBlockRule> | ref<PropertyDisplayRule> | ref<SchematicBusRule> | ref<SchematicForFrameImplementationRule> | ref<SchematicGlobalPortImplementationRule> | ref<SchematicIfFrameImplementationRule> | ref<SchematicInstanceImplementationRule> | ref<SchematicMasterPortImplementationRule> | ref<SchematicNetRule> | ref<SchematicOffPageConnectorImplementationRule> | ref<SchematicOnPageConnectorImplementationRule> | ref<SchematicOtherwiseFrameImplementationRule> | ref<SchematicRipperImplementationRule> | ref<UserDataRule> | ref<ViewPropertyDisplayRule>)));
  EDIF_FORM(PageAnnotateRule, "(pageAnnotate",
           arg(extendPageDefExpr) +
           many(arg(ref<InterconnectAnnotateRule>)));
  EDIF_FORM(PageBorderRule, "(pageBorder",
           arg(ref<PageBorderTemplateRefRule>) +
           arg(ref<TransformRule>) +
           many(arg(ref<PropertyDisplayOverrideRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(PageBorderTemplateRule, "(pageBorderTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeaderRule>) +
           arg(ref<UsableAreaRule>) +
           many(arg(ref<AnnotateRule> | ref<CommentGraphicsRule> | ref<FigureRule> | ref<PropertyDisplayRule> | ref<SchematicComplexFigureRule>)));
  EDIF_FORM(PageBorderTemplateRefRule, "(pageBorderTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(PageCommentGraphicsRule, "(pageCommentGraphics",
           many(arg(ref<AnnotateRule> | ref<CommentRule> | ref<FigureRule> | ref<SchematicComplexFigureRule> | ref<UserDataRule>)));
  EDIF_FORM(PageHeaderRule, "(pageHeader",
           many(arg(ref<BackgroundColorRule> | ref<DocumentationRule> | ref<NameInformationRule> | ref<OriginalBoundingBoxRule> | ref<PageBorderRule> | ref<PageSizeRule> | ref<PropertyRule> | ref<StatusRule>)));
  EDIF_FORM(PageIdentificationRule, "(pageIdentification",
           arg(stringTokenExpr));
  EDIF_FORM(PageIdentificationDisplayRule, "(pageIdentificationDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(PageNameCaseSensitiveRule, "(pageNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(PagePropertyDisplayRule, "(pagePropertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<DisplayRule> | ref<PropertyNameDisplayRule>)));
  EDIF_FORM(PageRefRule, "(pageRef",
           arg(pageNameRefExpr));
  EDIF_FORM(PageSizeRule, "(pageSize",
           arg(ref<RectangleRule>));
  EDIF_FORM(PageTitleRule, "(pageTitle",
           arg(stringTokenExpr));
  EDIF_FORM(PageTitleBlockRule, "(pageTitleBlock",
           arg(implementationNameDefExpr) +
           arg(ref<PageTitleBlockTemplateRefRule>) +
           arg(ref<TransformRule>) +
           many(arg(ref<NameInformationRule> | ref<PagePropertyDisplayRule> | ref<PageTitleBlockAttributeDisplayRule> | ref<PageTitleBlockAttributesRule> | ref<PropertyRule> | ref<PropertyDisplayRule> | ref<PropertyDisplayOverrideRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(PageTitleBlockAttributeDisplayRule, "(pageTitleBlockAttributeDisplay",
           many(arg(ref<ApprovedDateDisplayRule> | ref<CheckDateDisplayRule> | ref<CompanyNameDisplayRule> | ref<ContractDisplayRule> | ref<CopyrightDisplayRule> | ref<DrawingDescriptionDisplayRule> | ref<DrawingIdentificationDisplayRule> | ref<DrawingSizeDisplayRule> | ref<EngineeringDateDisplayRule> | ref<OriginalDrawingDateDisplayRule> | ref<PageIdentificationDisplayRule> | ref<PageTitleDisplayRule> | ref<RevisionDisplayRule> | ref<TotalPagesDisplayRule>)));
  EDIF_FORM(PageTitleBlockAttributesRule, "(pageTitleBlockAttributes",
           many(arg(ref<ApprovedDateRule> | ref<CheckDateRule> | ref<CompanyNameRule> | ref<ContractRule> | ref<DrawingDescriptionRule> | ref<DrawingIdentificationRule> | ref<DrawingSizeRule> | ref<EngineeringDateRule> | ref<OriginalDrawingDateRule> | ref<PageIdentificationRule> | ref<PageTitleRule> | ref<RevisionRule>)));
  EDIF_FORM(PageTitleBlockTemplateRule, "(pageTitleBlockTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeaderRule>) +
           many(arg(ref<AnnotateRule> | ref<CommentGraphicsRule> | ref<FigureRule> | ref<PageTitleBlockAttributeDisplayRule> | ref<PageTitleBlockAttributesRule> | ref<PropertyDisplayRule> | ref<SchematicComplexFigureRule>)));
  EDIF_FORM(PageTitleBlockTemplateRefRule, "(pageTitleBlockTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(PageTitleDisplayRule, "(pageTitleDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(ParameterDisplayRule, "(parameterDisplay",
           arg(parameterNameRefExpr) +
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>) +
           option(arg(ref<ParameterNameDisplayRule>)));
  EDIF_FORM(ParameterNameCaseSensitiveRule, "(parameterNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(ParameterNameDisplayRule, "(parameterNameDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(PathRule, "(path",
           arg(ref<PointListRule>));
  EDIF_FORM(PathDelayRule, "(pathDelay",
           arg(ref<DelayRule>) +
           many(arg(ref<EventRule>)));
  EDIF_FORM(PathWidthRule, "(pathWidth",
           arg(lengthValueExpr | ref<MinimalWidthRule>));
  EDIF_FORM(PcbLayoutUnitsRule, "(pcbLayoutUnits",
           many(arg(ref<SetAngleRule> | ref<SetCapacitanceRule> | ref<SetDistanceRule> | ref<SetTimeRule>)));
  EDIF_FORM(PcbLayoutViewRule, "(pcbLayoutView",
           arg(viewNameDefExpr) +
           many(arg(ref<CommentRule> | ref<NameInformationRule> | ref<UserDataRule>)));
  EDIF_FORM(PermutableRule, "(permutable",
           many(arg(ref<NonPermutableRule> | ref<PermutableRule> | ref<PortRefRule>)));
  EDIF_FORM(PhysicalDefaultsRule, "(physicalDefaults",
           option(arg(ref<SchematicRequiredDefaultsRule>)));
  EDIF_FORM(PhysicalScalingRule, "(physicalScaling",
           many(arg(ref<CommentRule> | ref<ConnectivityUnitsRule> | ref<DocumentationUnitsRule> | ref<GeometryMacroUnitsRule> | ref<InterfaceUnitsRule> | ref<LogicModelUnitsRule> | ref<MaskLayoutUnitsRule> | ref<PcbLayoutUnitsRule> | ref<SchematicUnitsRule> | ref<SymbolicLayoutUnitsRule>)));
  EDIF_FORM(PixelPatternRule, "(pixelPattern",
           arg(rowSizeExpr) +
           many(arg(ref<PixelRowRule>)));
  EDIF_FORM(PixelRowRule, "(pixelRow",
           many(arg(booleanTokenExpr)));
  EDIF_FORM(PointRule, "(point",
           arg(pointValueExpr));
  EDIF_FORM(PointListRule, "(pointList",
           many(arg(pointValueExpr)));
  EDIF_FORM(PolygonRule, "(polygon",
           arg(ref<PointListRule>));
  EDIF_FORM(PortRule, "(port",
           option(arg(portNameDefExpr)) +
           option(arg(portDirectionExpr)) +
           many(arg(ref<AcLoadRule> | ref<CommentRule> | ref<DefaultConnectionRule> | ref<DesignatorRule> | ref<NameInformationRule> | ref<PortDelayRule> | ref<PortLoadDelayRule> | ref<PortNameGeneratorRule> | ref<PortWidthRule> | ref<PropertyRule> | ref<SchematicPortAttributesRule> | ref<UnusedRule> | ref<UserDataRule>)));
  EDIF_FORM(PortAnnotateRule, "(portAnnotate",
           arg(extendPortDefExpr | extendPortMemberDefExpr) +
           many(arg(ref<AcLoadRule> | ref<CommentRule> | ref<DesignatorRule> | ref<DirectionalPortAttributeOverrideRule> | ref<PortDelayRule> | ref<PortDelayOverrideRule> | ref<PortLoadDelayRule> | ref<PortLoadDelayOverrideRule> | ref<PortPropertyOverrideRule> | ref<PropertyRule>)));
  EDIF_FORM(PortAttributeDisplayRule, "(portAttributeDisplay",
           many(arg(ref<AcLoadDisplayRule> | ref<ConnectedSignalIndexGeneratorDisplayRule> | ref<DcFanInLoadDisplayRule> | ref<DcFanOutLoadDisplayRule> | ref<DcMaxFanInDisplayRule> | ref<DcMaxFanOutDisplayRule> | ref<DesignatorDisplayRule> | ref<PortDelayDisplayRule> | ref<PortLoadDelayDisplayRule> | ref<PortNameDisplayRule> | ref<PortNameGeneratorDisplayRule> | ref<PortPropertyDisplayRule>)));
  EDIF_FORM(PortBundleRule, "(portBundle",
           arg(portNameDefExpr) +
           arg(ref<PortListRule>) +
           many(arg(ref<CommentRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<UserDataRule> | ref<DesignatorRule>)));
  EDIF_FORM(PortDelayRule, "(portDelay",
           arg(portDelayNameDefExpr) +
           arg(derivationExpr) +
           arg(ref<DelayRule>) +
           many(arg(ref<BecomesRule> | ref<TransitionRule>)));
  EDIF_FORM(PortDelayDisplayRule, "(portDelayDisplay",
           arg(portDelayNameRefExpr) +
           many(arg(ref<DisplayRule>)));
  EDIF_FORM(PortDelayOverrideRule, "(portDelayOverride",
           arg(portDelayNameRefExpr) +
           arg(derivationExpr) +
           arg(ref<DelayRule>) +
           many(arg(ref<BecomesRule> | ref<TransitionRule>)));
  EDIF_FORM0(PortIndexValueRule, "(portIndexValue");
  EDIF_FORM(PortInstanceRefRule, "(portInstanceRef",
           arg(portNameRefExpr | ref<PortMemberRefRule>) +
           arg(ref<InstanceRefRule> | ref<InstanceMemberRefRule>));
  EDIF_FORM(PortJoinedRule, "(portJoined",
           many(arg(ref<GlobalPortRefRule> | ref<LocalPortGroupRefRule> | ref<PortInstanceRefRule> | ref<PortRefRule>)));
  EDIF_FORM(PortListRule, "(portList",
           many(arg(ref<PortRefRule>)));
  EDIF_FORM(PortLoadDelayRule, "(portLoadDelay",
           arg(portLoadDelayNameDefExpr) +
           arg(derivationExpr) +
           arg(ref<LoadDelayRule>) +
           many(arg(ref<BecomesRule> | ref<TransitionRule>)));
  EDIF_FORM(PortLoadDelayDisplayRule, "(portLoadDelayDisplay",
           arg(portLoadDelayNameRefExpr) +
           many(arg(ref<DisplayRule>)));
  EDIF_FORM(PortLoadDelayOverrideRule, "(portLoadDelayOverride",
           arg(portLoadDelayNameRefExpr) +
           arg(derivationExpr) +
           arg(ref<LoadDelayRule>) +
           many(arg(ref<BecomesRule> | ref<TransitionRule>)));
  EDIF_FORM(PortMemberRefRule, "(portMemberRef",
           arg(portNameRefExpr) +
           arg(ref<IndexValueRule>));
  EDIF_FORM(PortNameCaseSensitiveRule, "(portNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(PortNameDisplayRule, "(portNameDisplay",
           many(arg(ref<DisplayRule> | ref<DisplayNameOverrideRule>)));
  EDIF_FORM(PortNameGeneratorRule, "(portNameGenerator",
           arg(stringExpressionExpr));
  EDIF_FORM(PortNameGeneratorDisplayRule, "(portNameGeneratorDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(PortPropertyDisplayRule, "(portPropertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<DisplayRule> | ref<PropertyNameDisplayRule>)));
  EDIF_FORM(PortPropertyDisplayOverrideRule, "(portPropertyDisplayOverride",
           arg(propertyNameRefExpr) +
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>) +
           option(arg(ref<PropertyNameDisplayRule>)));
  EDIF_FORM(PortPropertyOverrideRule, "(portPropertyOverride",
           arg(propertyNameRefExpr) +
           arg(typedValueExpr | ref<UntypedRule>) +
           many(arg(ref<CommentRule> | ref<FixedRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(PortRefRule, "(portRef",
           arg(portNameRefExpr));
  EDIF_FORM(PortSetRule, "(portSet",
           many(arg(ref<PortRefRule>)));
  EDIF_FORM(PortWidthRule, "(portWidth",
           arg(integerExpressionExpr));
  EDIF_FORM(PreviousVersionRule, "(previousVersion",
           arg(ref<ViewRefRule>) +
           option(arg(ref<ReasonRule>)));
  EDIF_FORM(PrimaryNameRule, "(primaryName",
           arg(originalNameExpr) +
           many(arg(ref<DisplayNameRule> | ref<GeneratedRule> | ref<NameStructureRule>)));
  EDIF_FORM(ProgramRule, "(program",
           arg(stringValueExpr) +
           option(arg(ref<VersionRule>)));
  EDIF_FORM(PropertyRule, "(property",
           arg(propertyNameDefExpr) +
           arg(typedValueExpr | ref<UntypedRule>) +
           many(arg(ref<CommentRule> | ref<NameInformationRule> | ref<OwnerRule> | ref<PropertyRule> | ref<PropertyInheritanceControlRule> | ref<UnitRefRule>)));
  EDIF_FORM(PropertyDisplayRule, "(propertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<DisplayRule> | ref<PropertyNameDisplayRule>)));
  EDIF_FORM(PropertyDisplayOverrideRule, "(propertyDisplayOverride",
           arg(propertyNameRefExpr) +
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>) +
           option(arg(ref<PropertyNameDisplayRule>)));
  EDIF_FORM(PropertyInheritanceControlRule, "(propertyInheritanceControl",
           arg(ref<FixedRule> | ref<NotInheritedRule>));
  EDIF_FORM(PropertyNameCaseSensitiveRule, "(propertyNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(PropertyNameDisplayRule, "(propertyNameDisplay",
           many(arg(ref<DisplayRule> | ref<DisplayNameOverrideRule>)));
  EDIF_FORM(PropertyOverrideRule, "(propertyOverride",
           arg(propertyNameRefExpr) +
           arg(typedValueExpr | ref<UntypedRule>) +
           many(arg(ref<CommentRule> | ref<FixedRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM0(ProportionalFontRule, "(proportionalFont");
  EDIF_FORM(PtRule, "(pt",
           arg(xCoordinateExpr) +
           arg(yCoordinateExpr));
  EDIF_FORM(RadianRule, "(radian",
           arg(unitExponentExpr));
  EDIF_FORM(ReasonRule, "(reason",
           arg(stringTokenExpr));
  EDIF_FORM(RectangleRule, "(rectangle",
           arg(pt1Expr) +
           arg(pt2Expr));
  EDIF_FORM0(RemoveDisplayRule, "(removeDisplay");
  EDIF_FORM(RepetitionCountRule, "(repetitionCount",
           arg(integerExpressionExpr));
  EDIF_FORM(RepetitionCountDisplayRule, "(repetitionCountDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(ReplaceDisplayRule, "(replaceDisplay",
           many(arg(ref<DisplayRule>)));
  EDIF_FORM0(RequiredRule, "(required");
  EDIF_FORM(ResolvesRule, "(resolves",
           many(arg(logicNameRefExpr)));
  EDIF_FORM(RevisionRule, "(revision",
           arg(stringTokenExpr));
  EDIF_FORM(RevisionDisplayRule, "(revisionDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM0(RightJustifyRule, "(rightJustify");
  EDIF_FORM(RipperHotspotRule, "(ripperHotspot",
           arg(hotspotNameDefExpr) +
           arg(ref<HotspotRule>));
  EDIF_FORM(RipperHotspotRefRule, "(ripperHotspotRef",
           arg(hotspotNameRefExpr) +
           arg(ref<SchematicRipperImplementationRefRule>));
  EDIF_FORM(RotationRule, "(rotation",
           arg(angleValueExpr));
  EDIF_FORM0(RoundRule, "(round");
  EDIF_FORM(ScaleXRule, "(scaleX",
           arg(numeratorExpr) +
           arg(denominatorExpr));
  EDIF_FORM(ScaleYRule, "(scaleY",
           arg(numeratorExpr) +
           arg(denominatorExpr));
  EDIF_FORM(SchematicBusRule, "(schematicBus",
           arg(interconnectNameDefExpr) +
           arg(ref<SignalGroupRefRule>) +
           arg(ref<SchematicInterconnectHeaderRule>) +
           arg(ref<SchematicBusJoinedRule>) +
           many(arg(ref<CommentRule> | ref<SchematicBusDetailsRule> | ref<SchematicBusSliceRule> | ref<SchematicInterconnectAttributeDisplayRule> | ref<UserDataRule>)));
  EDIF_FORM(SchematicBusDetailsRule, "(schematicBusDetails",
           arg(ref<SchematicBusGraphicsRule> | ref<SchematicSubBusSetRule>));
  EDIF_FORM(SchematicBusGraphicsRule, "(schematicBusGraphics",
           many(arg(ref<CommentRule> | ref<FigureRule> | ref<SchematicComplexFigureRule> | ref<UserDataRule>)));
  EDIF_FORM(SchematicBusJoinedRule, "(schematicBusJoined",
           many(arg(ref<PortJoinedRule> | ref<RipperHotspotRefRule> | ref<SchematicGlobalPortImplementationRefRule> | ref<SchematicInterconnectTerminatorImplementationRefRule> | ref<SchematicJunctionImplementationRefRule> | ref<SchematicMasterPortImplementationRefRule> | ref<SchematicOffPageConnectorImplementationRefRule> | ref<SchematicOnPageConnectorImplementationRefRule> | ref<SchematicSymbolPortImplementationRefRule>)));
  EDIF_FORM(SchematicBusSliceRule, "(schematicBusSlice",
           arg(interconnectNameDefExpr) +
           arg(ref<SignalGroupRefRule>) +
           arg(ref<SchematicInterconnectHeaderRule>) +
           arg(ref<SchematicBusJoinedRule>) +
           many(arg(ref<CommentRule> | ref<SchematicBusDetailsRule> | ref<SchematicBusSliceRule> | ref<SchematicInterconnectAttributeDisplayRule> | ref<UserDataRule>)));
  EDIF_FORM(SchematicComplexFigureRule, "(schematicComplexFigure",
           arg(ref<SchematicFigureMacroRefRule>) +
           arg(ref<TransformRule>) +
           many(arg(ref<PropertyDisplayOverrideRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(SchematicFigureMacroRule, "(schematicFigureMacro",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeaderRule>) +
           many(arg(ref<AnnotateRule> | ref<CommentRule> | ref<CommentGraphicsRule> | ref<FigureRule> | ref<PropertyDisplayRule> | ref<SchematicComplexFigureRule> | ref<UserDataRule>)));
  EDIF_FORM(SchematicFigureMacroRefRule, "(schematicFigureMacroRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(SchematicForFrameBorderRule, "(schematicForFrameBorder",
           arg(ref<SchematicForFrameBorderTemplateRefRule>) +
           arg(ref<TransformRule>) +
           many(arg(ref<ForFrameIndexDisplayRule> | ref<PropertyDisplayOverrideRule> | ref<PropertyOverrideRule> | ref<RepetitionCountDisplayRule>)));
  EDIF_FORM(SchematicForFrameBorderTemplateRule, "(schematicForFrameBorderTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeaderRule>) +
           arg(ref<UsableAreaRule>) +
           many(arg(ref<AnnotateRule> | ref<CommentGraphicsRule> | ref<FigureRule> | ref<ForFrameIndexDisplayRule> | ref<PropertyDisplayRule> | ref<RepetitionCountDisplayRule> | ref<SchematicComplexFigureRule>)));
  EDIF_FORM(SchematicForFrameBorderTemplateRefRule, "(schematicForFrameBorderTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(SchematicForFrameImplementationRule, "(schematicForFrameImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<ForFrameRefRule>) +
           arg(ref<SchematicForFrameImplementationHeaderRule>) +
           arg(ref<SchematicFrameImplementationDetailsRule>));
  EDIF_FORM(SchematicForFrameImplementationHeaderRule, "(schematicForFrameImplementationHeader",
           option(arg(ref<SchematicForFrameBorderRule>)));
  EDIF_FORM(SchematicFrameImplementationDetailsRule, "(schematicFrameImplementationDetails",
           many(arg(ref<CellPropertyDisplayRule> | ref<ClusterPropertyDisplayRule> | ref<CommentRule> | ref<CommentGraphicsRule> | ref<PropertyDisplayRule> | ref<SchematicBusRule> | ref<SchematicForFrameImplementationRule> | ref<SchematicGlobalPortImplementationRule> | ref<SchematicIfFrameImplementationRule> | ref<SchematicInstanceImplementationRule> | ref<SchematicMasterPortImplementationRule> | ref<SchematicNetRule> | ref<SchematicOffPageConnectorImplementationRule> | ref<SchematicOnPageConnectorImplementationRule> | ref<SchematicOtherwiseFrameImplementationRule> | ref<SchematicRipperImplementationRule> | ref<UserDataRule> | ref<ViewPropertyDisplayRule>)));
  EDIF_FORM(SchematicGlobalPortAttributesRule, "(schematicGlobalPortAttributes",
           many(arg(ref<IeeeStandardRule> | ref<SchematicPortAcPowerRule> | ref<SchematicPortAnalogRule> | ref<SchematicPortChassisGroundRule> | ref<SchematicPortClockRule> | ref<SchematicPortDcPowerRule> | ref<SchematicPortEarthGroundRule> | ref<SchematicPortGroundRule> | ref<SchematicPortNonLogicalRule> | ref<SchematicPortOpenCollectorRule> | ref<SchematicPortOpenEmitterRule> | ref<SchematicPortPowerRule> | ref<SchematicPortThreeStateRule>)));
  EDIF_FORM(SchematicGlobalPortImplementationRule, "(schematicGlobalPortImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<SchematicGlobalPortTemplateRefRule>) +
           arg(ref<GlobalPortRefRule>) +
           arg(ref<TransformRule>) +
           many(arg(ref<GlobalPortNameDisplayRule> | ref<GlobalPortPropertyDisplayRule> | ref<ImplementationNameDisplayRule> | ref<NameInformationRule> | ref<PropertyDisplayOverrideRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(SchematicGlobalPortImplementationRefRule, "(schematicGlobalPortImplementationRef",
           arg(implementationNameRefExpr));
  EDIF_FORM(SchematicGlobalPortTemplateRule, "(schematicGlobalPortTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeaderRule>) +
           option(arg(ref<HotspotRule>)) +
           many(arg(ref<AnnotateRule> | ref<CommentGraphicsRule> | ref<FigureRule> | ref<GlobalPortNameDisplayRule> | ref<ImplementationNameDisplayRule> | ref<PropertyDisplayRule> | ref<SchematicComplexFigureRule> | ref<SchematicGlobalPortAttributesRule>)) +
           option(arg(ref<HotspotRule>)));
  EDIF_FORM(SchematicGlobalPortTemplateRefRule, "(schematicGlobalPortTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(SchematicIfFrameBorderRule, "(schematicIfFrameBorder",
           arg(ref<SchematicIfFrameBorderTemplateRefRule>) +
           arg(ref<TransformRule>) +
           many(arg(ref<ConditionDisplayRule> | ref<PropertyDisplayOverrideRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(SchematicIfFrameBorderTemplateRule, "(schematicIfFrameBorderTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeaderRule>) +
           arg(ref<UsableAreaRule>) +
           many(arg(ref<AnnotateRule> | ref<CommentGraphicsRule> | ref<ConditionDisplayRule> | ref<FigureRule> | ref<PropertyDisplayRule> | ref<SchematicComplexFigureRule>)));
  EDIF_FORM(SchematicIfFrameBorderTemplateRefRule, "(schematicIfFrameBorderTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(SchematicIfFrameImplementationRule, "(schematicIfFrameImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<IfFrameRefRule>) +
           arg(ref<SchematicIfFrameImplementationHeaderRule>) +
           arg(ref<SchematicFrameImplementationDetailsRule>));
  EDIF_FORM(SchematicIfFrameImplementationHeaderRule, "(schematicIfFrameImplementationHeader",
           option(arg(ref<SchematicIfFrameBorderRule>)));
  EDIF_FORM(SchematicImplementationRule, "(schematicImplementation",
           many(arg(ref<PageRule> | ref<TotalPagesRule>)));
  EDIF_FORM(SchematicInstanceImplementationRule, "(schematicInstanceImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<InstanceRefRule>) +
           arg(ref<SchematicSymbolRefRule>) +
           arg(ref<TransformRule>) +
           many(arg(ref<CellNameDisplayRule> | ref<CellPropertyDisplayOverrideRule> | ref<ClusterPropertyDisplayOverrideRule> | ref<DesignatorDisplayRule> | ref<ImplementationNameDisplayRule> | ref<InstanceNameDisplayRule> | ref<InstanceNameGeneratorDisplayRule> | ref<InstancePortAttributeDisplayRule> | ref<InstancePropertyDisplayRule> | ref<InstanceWidthDisplayRule> | ref<NameInformationRule> | ref<PageCommentGraphicsRule> | ref<ParameterDisplayRule> | ref<PropertyDisplayOverrideRule> | ref<PropertyOverrideRule> | ref<TimingDisplayRule> | ref<ViewNameDisplayRule>)));
  EDIF_FORM(SchematicInstanceImplementationRefRule, "(schematicInstanceImplementationRef",
           arg(implementationNameRefExpr));
  EDIF_FORM(SchematicInterconnectAttributeDisplayRule, "(schematicInterconnectAttributeDisplay",
           many(arg(ref<ConnectivityTagGeneratorDisplayRule> | ref<CriticalityDisplayRule> | ref<InterconnectAttachedTextRule> | ref<InterconnectDelayDisplayRule> | ref<InterconnectNameDisplayRule> | ref<InterconnectPropertyDisplayRule>)));
  EDIF_FORM(SchematicInterconnectHeaderRule, "(schematicInterconnectHeader",
           many(arg(ref<CriticalityRule> | ref<DocumentationRule> | ref<InterconnectDelayRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<SchematicInterconnectTerminatorImplementationRule> | ref<SchematicJunctionImplementationRule> | ref<SchematicWireStyleRule>)));
  EDIF_FORM(SchematicInterconnectTerminatorImplementationRule, "(schematicInterconnectTerminatorImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<SchematicInterconnectTerminatorTemplateRefRule>) +
           arg(ref<TransformRule>) +
           many(arg(ref<ImplementationNameDisplayRule> | ref<NameInformationRule> | ref<PropertyDisplayOverrideRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(SchematicInterconnectTerminatorImplementationRefRule, "(schematicInterconnectTerminatorImplementationRef",
           arg(implementationNameRefExpr));
  EDIF_FORM(SchematicInterconnectTerminatorTemplateRule, "(schematicInterconnectTerminatorTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeaderRule>) +
           arg(ref<HotspotRule>) +
           many(arg(ref<CommentGraphicsRule> | ref<FigureRule> | ref<ImplementationNameDisplayRule> | ref<PropertyDisplayRule> | ref<SchematicComplexFigureRule>)));
  EDIF_FORM(SchematicInterconnectTerminatorTemplateRefRule, "(schematicInterconnectTerminatorTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(SchematicJunctionImplementationRule, "(schematicJunctionImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<SchematicJunctionTemplateRefRule>) +
           arg(ref<TransformRule>) +
           many(arg(ref<ImplementationNameDisplayRule> | ref<NameInformationRule> | ref<PropertyDisplayOverrideRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(SchematicJunctionImplementationRefRule, "(schematicJunctionImplementationRef",
           arg(implementationNameRefExpr));
  EDIF_FORM(SchematicJunctionTemplateRule, "(schematicJunctionTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeaderRule>) +
           arg(ref<HotspotRule>) +
           many(arg(ref<CommentGraphicsRule> | ref<FigureRule> | ref<ImplementationNameDisplayRule> | ref<PropertyDisplayRule> | ref<SchematicComplexFigureRule>)));
  EDIF_FORM(SchematicJunctionTemplateRefRule, "(schematicJunctionTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(SchematicMasterPortImplementationRule, "(schematicMasterPortImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<SchematicMasterPortTemplateRefRule>) +
           arg(ref<PortRefRule> | ref<LocalPortGroupRefRule>) +
           arg(ref<TransformRule>) +
           many(arg(ref<ImplementationNameDisplayRule> | ref<NameInformationRule> | ref<PortAttributeDisplayRule> | ref<PropertyDisplayOverrideRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(SchematicMasterPortImplementationRefRule, "(schematicMasterPortImplementationRef",
           arg(implementationNameRefExpr));
  EDIF_FORM(SchematicMasterPortTemplateRule, "(schematicMasterPortTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeaderRule>) +
           arg(ref<HotspotRule>) +
           arg(portDirectionIndicatorExpr) +
           many(arg(ref<AnnotateRule> | ref<CommentGraphicsRule> | ref<FigureRule> | ref<ImplementationNameDisplayRule> | ref<PortAttributeDisplayRule> | ref<PropertyDisplayRule> | ref<SchematicComplexFigureRule> | ref<SchematicPortStyleRule>)));
  EDIF_FORM(SchematicMasterPortTemplateRefRule, "(schematicMasterPortTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(SchematicMetricRule, "(schematicMetric",
           arg(ref<SetDistanceRule>) +
           arg(ref<HotspotGridRule> | ref<NoHotspotGridRule>) +
           option(arg(ref<NominalHotspotGridRule>)));
  EDIF_FORM(SchematicNetRule, "(schematicNet",
           arg(interconnectNameDefExpr) +
           arg(ref<SignalRefRule>) +
           arg(ref<SchematicInterconnectHeaderRule>) +
           arg(ref<SchematicNetJoinedRule>) +
           many(arg(ref<CommentRule> | ref<SchematicInterconnectAttributeDisplayRule> | ref<SchematicNetDetailsRule> | ref<UserDataRule>)));
  EDIF_FORM(SchematicNetDetailsRule, "(schematicNetDetails",
           arg(ref<SchematicNetGraphicsRule> | ref<SchematicSubNetSetRule>));
  EDIF_FORM(SchematicNetGraphicsRule, "(schematicNetGraphics",
           many(arg(ref<CommentRule> | ref<FigureRule> | ref<SchematicComplexFigureRule> | ref<UserDataRule>)));
  static constexpr auto schematicNetJoinedRefExpr = ref<RipperHotspotRefRule> | ref<SchematicGlobalPortImplementationRefRule> | ref<SchematicInterconnectTerminatorImplementationRefRule> | ref<SchematicJunctionImplementationRefRule> | ref<SchematicMasterPortImplementationRefRule> | ref<SchematicOffPageConnectorImplementationRefRule> | ref<SchematicOnPageConnectorImplementationRefRule> | ref<SchematicSymbolPortImplementationRefRule>;
  static constexpr auto schematicNetJoinedPortExpr = ref<PortJoinedRule> | ref<JoinedAsSignalRule>;
  EDIF_FORM(SchematicNetJoinedRule, "(schematicNetJoined",
           option((some(arg(schematicNetJoinedRefExpr)) |
                   (arg(schematicNetJoinedPortExpr) +
                    many(arg(schematicNetJoinedRefExpr)))) +
                  option(arg(schematicNetJoinedPortExpr) +
                         many(arg(schematicNetJoinedRefExpr)))));
  EDIF_FORM(SchematicOffPageConnectorImplementationRule, "(schematicOffPageConnectorImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<SchematicOffPageConnectorTemplateRefRule>) +
           arg(ref<TransformRule>) +
           many(arg(ref<AssociatedInterconnectNameDisplayRule> | ref<ImplementationNameDisplayRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<PropertyDisplayRule> | ref<PropertyDisplayOverrideRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(SchematicOffPageConnectorImplementationRefRule, "(schematicOffPageConnectorImplementationRef",
           arg(implementationNameRefExpr));
  EDIF_FORM(SchematicOffPageConnectorTemplateRule, "(schematicOffPageConnectorTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeaderRule>) +
           arg(ref<HotspotRule>) +
           many(arg(ref<AnnotateRule> | ref<AssociatedInterconnectNameDisplayRule> | ref<CommentGraphicsRule> | ref<FigureRule> | ref<ImplementationNameDisplayRule> | ref<PropertyDisplayRule> | ref<SchematicComplexFigureRule>)));
  EDIF_FORM(SchematicOffPageConnectorTemplateRefRule, "(schematicOffPageConnectorTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(SchematicOnPageConnectorImplementationRule, "(schematicOnPageConnectorImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<SchematicOnPageConnectorTemplateRefRule>) +
           arg(ref<TransformRule>) +
           many(arg(ref<AssociatedInterconnectNameDisplayRule> | ref<ImplementationNameDisplayRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<PropertyDisplayRule> | ref<PropertyDisplayOverrideRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(SchematicOnPageConnectorImplementationRefRule, "(schematicOnPageConnectorImplementationRef",
           arg(implementationNameRefExpr));
  EDIF_FORM(SchematicOnPageConnectorTemplateRule, "(schematicOnPageConnectorTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeaderRule>) +
           arg(ref<HotspotRule>) +
           many(arg(ref<AnnotateRule> | ref<AssociatedInterconnectNameDisplayRule> | ref<CommentGraphicsRule> | ref<FigureRule> | ref<ImplementationNameDisplayRule> | ref<PropertyDisplayRule> | ref<SchematicComplexFigureRule>)));
  EDIF_FORM(SchematicOnPageConnectorTemplateRefRule, "(schematicOnPageConnectorTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(SchematicOtherwiseFrameBorderRule, "(schematicOtherwiseFrameBorder",
           arg(ref<SchematicOtherwiseFrameBorderTemplateRefRule>) +
           arg(ref<TransformRule>) +
           many(arg(ref<PropertyDisplayOverrideRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(SchematicOtherwiseFrameBorderTemplateRule, "(schematicOtherwiseFrameBorderTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeaderRule>) +
           arg(ref<UsableAreaRule>) +
           many(arg(ref<AnnotateRule> | ref<CommentGraphicsRule> | ref<FigureRule> | ref<PropertyDisplayRule> | ref<SchematicComplexFigureRule>)));
  EDIF_FORM(SchematicOtherwiseFrameBorderTemplateRefRule, "(schematicOtherwiseFrameBorderTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(SchematicOtherwiseFrameImplementationRule, "(schematicOtherwiseFrameImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<OtherwiseFrameRefRule>) +
           arg(ref<SchematicOtherwiseFrameImplementationHeaderRule>) +
           arg(ref<SchematicFrameImplementationDetailsRule>));
  EDIF_FORM(SchematicOtherwiseFrameImplementationHeaderRule, "(schematicOtherwiseFrameImplementationHeader",
           option(arg(ref<SchematicOtherwiseFrameBorderRule>)));
  EDIF_FORM(SchematicPortAcPowerRule, "(schematicPortAcPower",
           many(arg(ref<SchematicPortAcPowerRecommendedFrequencyRule> | ref<SchematicPortAcPowerRecommendedVoltageRmsRule>)));
  EDIF_FORM(SchematicPortAcPowerRecommendedFrequencyRule, "(schematicPortAcPowerRecommendedFrequency",
           arg(frequencyValueExpr));
  EDIF_FORM(SchematicPortAcPowerRecommendedVoltageRmsRule, "(schematicPortAcPowerRecommendedVoltageRms",
           arg(voltageValueExpr));
  EDIF_FORM0(SchematicPortAnalogRule, "(schematicPortAnalog");
  EDIF_FORM(SchematicPortAttributesRule, "(schematicPortAttributes",
           many(arg(ref<IeeeStandardRule> | ref<SchematicPortAcPowerRule> | ref<SchematicPortAnalogRule> | ref<SchematicPortChassisGroundRule> | ref<SchematicPortClockRule> | ref<SchematicPortDcPowerRule> | ref<SchematicPortEarthGroundRule> | ref<SchematicPortGroundRule> | ref<SchematicPortNonLogicalRule> | ref<SchematicPortOpenCollectorRule> | ref<SchematicPortOpenEmitterRule> | ref<SchematicPortPowerRule> | ref<SchematicPortThreeStateRule>)));
  EDIF_FORM(SchematicPortChassisGroundRule, "(schematicPortChassisGround",
           option(arg(ref<SchematicPortAnalogRule>)));
  EDIF_FORM(SchematicPortClockRule, "(schematicPortClock",
           option(arg(ref<IeeeStandardRule>)));
  EDIF_FORM(SchematicPortDcPowerRule, "(schematicPortDcPower",
           many(arg(ref<SchematicPortAnalogRule> | ref<SchematicPortDcPowerRecommendedVoltageRule>)));
  EDIF_FORM(SchematicPortDcPowerRecommendedVoltageRule, "(schematicPortDcPowerRecommendedVoltage",
           arg(voltageValueExpr));
  EDIF_FORM(SchematicPortEarthGroundRule, "(schematicPortEarthGround",
           option(arg(ref<SchematicPortAnalogRule>)));
  EDIF_FORM(SchematicPortGroundRule, "(schematicPortGround",
           option(arg(ref<SchematicPortAnalogRule>)));
  EDIF_FORM0(SchematicPortNonLogicalRule, "(schematicPortNonLogical");
  EDIF_FORM0(SchematicPortOpenCollectorRule, "(schematicPortOpenCollector");
  EDIF_FORM0(SchematicPortOpenEmitterRule, "(schematicPortOpenEmitter");
  EDIF_FORM0(SchematicPortPowerRule, "(schematicPortPower");
  EDIF_FORM(SchematicPortStyleRule, "(schematicPortStyle",
           arg(ref<NarrowPortRule> | ref<WidePortRule>));
  EDIF_FORM0(SchematicPortThreeStateRule, "(schematicPortThreeState");
  EDIF_FORM(SchematicRequiredDefaultsRule, "(schematicRequiredDefaults",
           arg(ref<SchematicMetricRule>) +
           arg(ref<FontRefRule>) +
           arg(ref<TextHeightRule>));
  EDIF_FORM(SchematicRipperImplementationRule, "(schematicRipperImplementation",
           arg(implementationNameDefExpr) +
           arg(ref<SchematicRipperTemplateRefRule>) +
           arg(ref<TransformRule>) +
           many(arg(ref<ImplementationNameDisplayRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<PropertyDisplayRule> | ref<PropertyDisplayOverrideRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(SchematicRipperImplementationRefRule, "(schematicRipperImplementationRef",
           arg(implementationNameRefExpr));
  EDIF_FORM(SchematicRipperTemplateRule, "(schematicRipperTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeaderRule>) +
           many(arg(ref<AnnotateRule> | ref<CommentGraphicsRule> | ref<FigureRule> | ref<ImplementationNameDisplayRule> | ref<PropertyDisplayRule> | ref<RipperHotspotRule> | ref<SchematicComplexFigureRule>)));
  EDIF_FORM(SchematicRipperTemplateRefRule, "(schematicRipperTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(SchematicSubBusRule, "(schematicSubBus",
           arg(interconnectNameDefExpr) +
           arg(ref<SchematicSubInterconnectHeaderRule>) +
           arg(ref<SchematicBusJoinedRule>) +
           many(arg(ref<CommentRule> | ref<SchematicBusDetailsRule> | ref<SchematicBusSliceRule> | ref<SchematicInterconnectAttributeDisplayRule> | ref<UserDataRule>)));
  EDIF_FORM(SchematicSubBusSetRule, "(schematicSubBusSet",
           many(arg(ref<SchematicSubBusRule>)));
  EDIF_FORM(SchematicSubInterconnectHeaderRule, "(schematicSubInterconnectHeader",
           many(arg(ref<CriticalityRule> | ref<DocumentationRule> | ref<InterconnectDelayRule> | ref<NameInformationRule> | ref<PropertyRule>)));
  EDIF_FORM(SchematicSubNetRule, "(schematicSubNet",
           arg(interconnectNameDefExpr) +
           arg(ref<SchematicSubInterconnectHeaderRule>) +
           arg(ref<SchematicNetJoinedRule>) +
           many(arg(ref<CommentRule> | ref<SchematicInterconnectAttributeDisplayRule> | ref<SchematicNetDetailsRule> | ref<UserDataRule>)));
  EDIF_FORM(SchematicSubNetSetRule, "(schematicSubNetSet",
           many(arg(ref<SchematicSubNetRule>)));
  EDIF_FORM(SchematicSymbolRule, "(schematicSymbol",
           arg(viewNameDefExpr) +
           arg(ref<SchematicSymbolHeaderRule>) +
           many(arg(ref<AnnotateRule> | ref<CellNameDisplayRule> | ref<CellPropertyDisplayRule> | ref<ClusterPropertyDisplayRule> | ref<CommentRule> | ref<CommentGraphicsRule> | ref<DesignatorDisplayRule> | ref<FigureRule> | ref<ImplementationNameDisplayRule> | ref<InstanceNameDisplayRule> | ref<InstanceNameGeneratorDisplayRule> | ref<InstanceWidthDisplayRule> | ref<ParameterDisplayRule> | ref<PropertyDisplayRule> | ref<SchematicComplexFigureRule> | ref<SchematicSymbolPortImplementationRule> | ref<UserDataRule> | ref<ViewNameDisplayRule>)));
  EDIF_FORM(SchematicSymbolBorderRule, "(schematicSymbolBorder",
           arg(ref<SchematicSymbolBorderTemplateRefRule>) +
           arg(ref<TransformRule>) +
           many(arg(ref<PropertyDisplayOverrideRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(SchematicSymbolBorderTemplateRule, "(schematicSymbolBorderTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeaderRule>) +
           arg(ref<UsableAreaRule>) +
           many(arg(ref<AnnotateRule> | ref<CommentGraphicsRule> | ref<FigureRule> | ref<PropertyDisplayRule> | ref<SchematicComplexFigureRule>)));
  EDIF_FORM(SchematicSymbolBorderTemplateRefRule, "(schematicSymbolBorderTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(SchematicSymbolHeaderRule, "(schematicSymbolHeader",
           arg(ref<SchematicUnitsRule>) +
           many(arg(ref<BackgroundColorRule> | ref<DerivedFromRule> | ref<DocumentationRule> | ref<NameInformationRule> | ref<OriginalBoundingBoxRule> | ref<PageSizeRule> | ref<PreviousVersionRule> | ref<PropertyRule> | ref<SchematicSymbolBorderRule> | ref<StatusRule>)));
  EDIF_FORM(SchematicSymbolPortImplementationRule, "(schematicSymbolPortImplementation",
           arg(symbolPortImplementationNameDefExpr) +
           arg(ref<PortRefRule>) +
           arg(ref<SchematicSymbolPortTemplateRefRule>) +
           arg(ref<TransformRule>) +
           many(arg(ref<PortAttributeDisplayRule> | ref<PropertyDisplayOverrideRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(SchematicSymbolPortImplementationRefRule, "(schematicSymbolPortImplementationRef",
           arg(symbolPortImplementationNameRefExpr) +
           arg(ref<SchematicInstanceImplementationRefRule>));
  EDIF_FORM(SchematicSymbolPortTemplateRule, "(schematicSymbolPortTemplate",
           arg(libraryObjectNameDefExpr) +
           arg(ref<SchematicTemplateHeaderRule>) +
           arg(ref<HotspotRule>) +
           arg(portDirectionIndicatorExpr) +
           many(arg(ref<AnnotateRule> | ref<CommentGraphicsRule> | ref<FigureRule> | ref<ImplementationNameDisplayRule> | ref<PortAttributeDisplayRule> | ref<PropertyDisplayRule> | ref<SchematicComplexFigureRule> | ref<SchematicPortAttributesRule> | ref<SchematicPortStyleRule>)));
  EDIF_FORM(SchematicSymbolPortTemplateRefRule, "(schematicSymbolPortTemplateRef",
           arg(libraryObjectNameRefExpr) +
           option(arg(ref<LibraryRefRule>)));
  EDIF_FORM(SchematicSymbolRefRule, "(schematicSymbolRef",
           arg(viewNameRefExpr));
  EDIF_FORM(SchematicTemplateHeaderRule, "(schematicTemplateHeader",
           arg(ref<SchematicUnitsRule>) +
           many(arg(ref<BackgroundColorRule> | ref<DocumentationRule> | ref<NameInformationRule> | ref<OriginalBoundingBoxRule> | ref<PropertyRule> | ref<StatusRule>)));
  EDIF_FORM(SchematicUnitsRule, "(schematicUnits",
           many(arg(ref<SchematicMetricRule> | ref<SetAngleRule> | ref<SetCapacitanceRule> | ref<SetFrequencyRule> | ref<SetTimeRule> | ref<SetVoltageRule>)));
  EDIF_FORM(SchematicViewRule, "(schematicView",
           arg(viewNameDefExpr) +
           arg(ref<SchematicViewHeaderRule>) +
           arg(ref<LogicalConnectivityRule>) +
           arg(ref<SchematicImplementationRule>) +
           many(arg(ref<CommentRule> | ref<UserDataRule>)));
  EDIF_FORM(SchematicViewHeaderRule, "(schematicViewHeader",
           arg(ref<SchematicUnitsRule>) +
           many(arg(ref<DerivedFromRule> | ref<DocumentationRule> | ref<NameInformationRule> | ref<PreviousVersionRule> | ref<PropertyRule> | ref<StatusRule>)));
  EDIF_FORM(SchematicWireAffinityRule, "(schematicWireAffinity",
           arg(ref<NarrowWireRule> | ref<WideWireRule> | ref<UnrestrictedRule>));
  EDIF_FORM(SchematicWireStyleRule, "(schematicWireStyle",
           arg(ref<NarrowWireRule> | ref<WideWireRule>));
  EDIF_FORM(SecondRule, "(second",
           arg(unitExponentExpr));
  EDIF_FORM(SectionRule, "(section",
           arg(sectionNameDefExpr) +
           arg(ref<SectionTitleRule>) +
           many(arg(ref<DiagramRule> | ref<SectionRule> | stringValueExpr)));
  EDIF_FORM(SectionTitleRule, "(sectionTitle",
           arg(stringTokenExpr));
  EDIF_FORM(SequenceRule, "(sequence",
           arg(fromIntegerExpr) +
           arg(toIntegerExpr) +
           option(arg(ref<StepRule>)));
  EDIF_FORM(SetAngleRule, "(setAngle",
           arg(ref<UnitRefRule>));
  EDIF_FORM(SetCapacitanceRule, "(setCapacitance",
           arg(ref<UnitRefRule>));
  EDIF_FORM(SetCurrentRule, "(setCurrent",
           arg(ref<UnitRefRule>));
  EDIF_FORM(SetDistanceRule, "(setDistance",
           arg(ref<UnitRefRule>));
  EDIF_FORM(SetFrequencyRule, "(setFrequency",
           arg(ref<UnitRefRule>));
  EDIF_FORM(SetTimeRule, "(setTime",
           arg(ref<UnitRefRule>));
  EDIF_FORM(SetVoltageRule, "(setVoltage",
           arg(ref<UnitRefRule>));
  EDIF_FORM(ShapeRule, "(shape",
           arg(ref<CurveRule>));
  EDIF_FORM(SiemensRule, "(siemens",
           arg(unitExponentExpr));
  EDIF_FORM(SignalRule, "(signal",
           arg(signalNameDefExpr) +
           arg(ref<SignalJoinedRule>) +
           many(arg(ref<ConnectivityTagGeneratorRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<SignalWidthRule>)));
  EDIF_FORM(SignalAnnotateRule, "(signalAnnotate",
           arg(extendSignalDefExpr | extendSignalMemberDefExpr) +
           many(arg(ref<CommentRule> | ref<PropertyRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(SignalGroupRule, "(signalGroup",
           arg(signalGroupNameDefExpr) +
           arg(ref<SignalListRule>) +
           many(arg(ref<PropertyRule> | ref<NameInformationRule>)));
  EDIF_FORM(SignalGroupAnnotateRule, "(signalGroupAnnotate",
           arg(extendSignalGroupDefExpr) +
           many(arg(ref<CommentRule> | ref<PropertyRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(SignalGroupNameCaseSensitiveRule, "(signalGroupNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(SignalGroupRefRule, "(signalGroupRef",
           arg(signalGroupNameRefExpr));
  EDIF_FORM0(SignalIndexValueRule, "(signalIndexValue");
  EDIF_FORM(SignalJoinedRule, "(signalJoined",
           many(arg(ref<GlobalPortRefRule> | ref<PortInstanceRefRule> | ref<PortMemberRefRule> | ref<PortRefRule>)));
  EDIF_FORM(SignalListRule, "(signalList",
           many(arg(ref<SignalGroupRefRule> | ref<SignalRefRule>)));
  EDIF_FORM(SignalMemberRefRule, "(signalMemberRef",
           arg(signalNameRefExpr) +
           arg(ref<IndexValueRule>));
  EDIF_FORM(SignalNameCaseSensitiveRule, "(signalNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(SignalRefRule, "(signalRef",
           arg(signalNameRefExpr));
  EDIF_FORM(SignalWidthRule, "(signalWidth",
           arg(integerExpressionExpr));
  EDIF_FORM(StatusRule, "(status",
           many(arg(ref<CommentRule> | ref<CopyrightRule> | ref<UserDataRule> | ref<WrittenRule>)));
  EDIF_FORM(StepRule, "(step",
           arg(integerValueExpr));
  EDIF_FORM(StringRule, "(string",
           arg(stringExpressionExpr));
  EDIF_FORM(StringConcatenateRule, "(stringConcatenate",
           many(arg(stringExpressionExpr)));
  EDIF_FORM(StringConstantRule, "(stringConstant",
           arg(constantNameDefExpr) +
           arg(stringTokenExpr));
  EDIF_FORM(StringConstantRefRule, "(stringConstantRef",
           arg(constantNameRefExpr));
  EDIF_FORM(StringEqualRule, "(stringEqual",
           arg(firstStringExpressionExpr) +
           arg(secondStringExpressionExpr));
  EDIF_FORM(StringLengthRule, "(stringLength",
           arg(stringExpressionExpr));
  EDIF_FORM(StringParameterRule, "(stringParameter",
           arg(parameterNameDefExpr) +
           many(arg(ref<NameInformationRule> | ref<StringRule>)));
  EDIF_FORM(StringParameterAssignRule, "(stringParameterAssign",
           arg(parameterNameRefExpr) +
           arg(stringExpressionExpr));
  EDIF_FORM(StringParameterRefRule, "(stringParameterRef",
           arg(parameterNameRefExpr));
  EDIF_FORM(StringPrefixRule, "(stringPrefix",
           arg(stringExpressionExpr) +
           arg(substringLengthExpr));
  EDIF_FORM(StringSuffixRule, "(stringSuffix",
           arg(stringExpressionExpr) +
           arg(substringOffsetExpr));
  EDIF_FORM(StrongRule, "(strong",
           arg(logicNameRefExpr));
  EDIF_FORM(SubstringRule, "(substring",
           arg(stringExpressionExpr) +
           arg(substringLengthExpr) +
           arg(substringOffsetExpr));
  EDIF_FORM(SymbolicLayoutUnitsRule, "(symbolicLayoutUnits",
           many(arg(ref<SetAngleRule> | ref<SetCapacitanceRule> | ref<SetDistanceRule> | ref<SetTimeRule>)));
  EDIF_FORM(SymbolicLayoutViewRule, "(symbolicLayoutView",
           arg(viewNameDefExpr) +
           many(arg(ref<CommentRule> | ref<UserDataRule>)) +
           option(arg(ref<NameInformationRule>) +
                  many(arg(ref<CommentRule> | ref<UserDataRule>))));
  EDIF_FORM(TechnologyRule, "(technology",
           arg(ref<PhysicalScalingRule>) +
           many(arg(ref<CommentRule> | ref<FigureGroupRule> | ref<LogicDefinitionsRule> | ref<UserDataRule>)));
  EDIF_FORM(TextHeightRule, "(textHeight",
           arg(lengthValueExpr));
  EDIF_FORM(TimeRule, "(time",
           arg(hourNumberExpr) +
           arg(minuteNumberExpr) +
           arg(secondNumberExpr));
  EDIF_FORM(TimeIntervalRule, "(timeInterval",
           arg(ref<EventRule> | ref<OffsetEventRule>) +
           arg(ref<EventRule> | ref<OffsetEventRule> | ref<DurationRule>));
  EDIF_FORM(TimeStampRule, "(timeStamp",
           arg(ref<DateRule>) +
           arg(ref<TimeRule>));
  EDIF_FORM(TimingRule, "(timing",
           arg(timingNameDefExpr) +
           arg(derivationExpr) +
           many(arg(ref<CommentRule> | ref<ForbiddenEventRule> | ref<PathDelayRule> | ref<PropertyRule> | ref<UserDataRule>)));
  EDIF_FORM(TimingDisplayRule, "(timingDisplay",
           arg(timingNameRefExpr) +
           many(arg(ref<DisplayRule>)));
  EDIF_FORM0(TopJustifyRule, "(topJustify");
  EDIF_FORM(TotalPagesRule, "(totalPages",
           arg(integerTokenExpr));
  EDIF_FORM(TotalPagesDisplayRule, "(totalPagesDisplay",
           arg(ref<AddDisplayRule> | ref<ReplaceDisplayRule> | ref<RemoveDisplayRule>));
  EDIF_FORM(TransformRule, "(transform",
           many(arg(ref<OriginRule> | ref<RotationRule> | ref<ScaleXRule> | ref<ScaleYRule>)));
  EDIF_FORM(TransitionRule, "(transition",
           arg(previousLogicValueExpr) +
           arg(presentLogicValueExpr));
  EDIF_FORM0(EtrueRule, "(true");
  EDIF_FORM0(TruncateRule, "(truncate");
  EDIF_FORM(TypefaceRule, "(typeface",
           arg(fontFamilyExpr) +
           option(arg(ref<TypefaceSuffixRule>)));
  EDIF_FORM(TypefaceSuffixRule, "(typefaceSuffix",
           arg(stringTokenExpr));
  EDIF_FORM0(UnconfiguredRule, "(unconfigured");
  EDIF_FORM0(UnconstrainedRule, "(unconstrained");
  EDIF_FORM0(UndefinedRule, "(undefined");
  EDIF_FORM(UnitRule, "(unit",
           arg(unitNameDefExpr) +
           arg(numberOfNewUnitsExpr) +
           arg(numberOfBasicUnitsExpr) +
           many(arg(ref<AmpereRule> | ref<CandelaRule> | ref<CelsiusRule> | ref<CoulombRule> | ref<DegreeRule> | ref<FahrenheitRule> | ref<FaradRule> | ref<HenryRule> | ref<HertzRule> | ref<JouleRule> | ref<KelvinRule> | ref<KilogramRule> | ref<MeterRule> | ref<MoleRule> | ref<OhmRule> | ref<RadianRule> | ref<SecondRule> | ref<SiemensRule> | ref<VoltRule> | ref<WattRule> | ref<WeberRule>)));
  EDIF_FORM(UnitDefinitionsRule, "(unitDefinitions",
           many(arg(ref<UnitRule>)));
  EDIF_FORM(UnitRefRule, "(unitRef",
           arg(unitNameRefExpr));
  EDIF_FORM0(UnrestrictedRule, "(unrestricted");
  EDIF_FORM0(UnspecifiedRule, "(unspecified");
  EDIF_FORM0(UnspecifiedDirectionPortRule, "(unspecifiedDirectionPort");
  EDIF_FORM0(UntypedRule, "(untyped");
  EDIF_FORM0(UnusedRule, "(unused");
  EDIF_FORM(UsableAreaRule, "(usableArea",
           many(arg(ref<RectangleRule>)));
  EDIF_FORM(UserDataRule, "(userData",
           arg(userDataTagExpr) +
           many(arg(ref<IDENTIFIER> | integerTokenExpr | stringTokenExpr | ref<UserDataRule>)));
  EDIF_FORM(VersionRule, "(version",
           arg(stringValueExpr));
  EDIF_FORM(VerticalJustificationRule, "(verticalJustification",
           arg(ref<BottomJustifyRule> | ref<BaselineJustifyRule> | ref<MiddleJustifyRule> | ref<CaplineJustifyRule> | ref<TopJustifyRule>));
  EDIF_FORM(ViewGroupRule, "(viewGroup",
           arg(viewGroupNameDefExpr) +
           arg(ref<ViewGroupHeaderRule>) +
           many(arg(ref<CommentRule> | ref<UserDataRule> | ref<ViewGroupRefRule> | ref<ViewRefRule>)));
  EDIF_FORM(ViewGroupHeaderRule, "(viewGroupHeader",
           many(arg(ref<DocumentationRule> | ref<NameInformationRule> | ref<PropertyRule> | ref<ReasonRule>)));
  EDIF_FORM(ViewGroupNameCaseSensitiveRule, "(viewGroupNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(ViewGroupRefRule, "(viewGroupRef",
           arg(viewGroupNameRefExpr));
  EDIF_FORM(ViewNameCaseSensitiveRule, "(viewNameCaseSensitive",
           arg(booleanTokenExpr));
  EDIF_FORM(ViewNameDisplayRule, "(viewNameDisplay",
           many(arg(ref<DisplayRule> | ref<DisplayNameOverrideRule>)));
  EDIF_FORM(ViewPropertyDisplayRule, "(viewPropertyDisplay",
           arg(propertyNameRefExpr) +
           many(arg(ref<DisplayRule> | ref<PropertyNameDisplayRule>)));
  EDIF_FORM(ViewPropertyOverrideRule, "(viewPropertyOverride",
           arg(propertyNameRefExpr) +
           arg(typedValueExpr | ref<UntypedRule>) +
           many(arg(ref<CommentRule> | ref<FixedRule> | ref<PropertyOverrideRule>)));
  EDIF_FORM(ViewRefRule, "(viewRef",
           arg(viewNameRefExpr) +
           option(arg(ref<ClusterRefRule>)));
  EDIF_FORM(VisibleRule, "(visible",
           arg(booleanValueExpr));
  EDIF_FORM(VoltRule, "(volt",
           arg(unitExponentExpr));
  EDIF_FORM(VoltageMapRule, "(voltageMap",
           arg(voltageValueExpr));
  EDIF_FORM(WattRule, "(watt",
           arg(unitExponentExpr));
  EDIF_FORM(WeakRule, "(weak",
           arg(logicNameRefExpr));
  EDIF_FORM(WeakJoinedRule, "(weakJoined",
           many(arg(ref<InterfaceJoinedRule> | ref<PortRefRule>)));
  EDIF_FORM(WeberRule, "(weber",
           arg(unitExponentExpr));
  EDIF_FORM0(WidePortRule, "(widePort");
  EDIF_FORM0(WideWireRule, "(wideWire");
  EDIF_FORM(WrittenRule, "(written",
           arg(ref<TimeStampRule>) +
           many(arg(ref<AuthorRule> | ref<CommentRule> | ref<DataOriginRule> | ref<ProgramRule> | ref<UserDataRule>)));
  EDIF_FORM(XorRule, "(xor",
           many(arg(booleanExpressionExpr)));
  EDIF_FORM(YearRule, "(year",
           many(arg(yearNumberExpr)));

#undef EDIF_FORM0
#undef EDIF_FORM

  // goal: edif EOF -- the whole document is one (edif ...) form.
  struct GoalRule : rule<ref<EdifRule> + eof> {};

  using entry = GoalRule;
  using ignore = WS;
};

}  // namespace edif300
