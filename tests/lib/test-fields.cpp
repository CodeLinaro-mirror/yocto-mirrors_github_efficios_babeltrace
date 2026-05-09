/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2023 EfficiOS Inc.
 */

#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>

#include "cpp-common/bt2/field-class.hpp"
#include "cpp-common/bt2/integer-range-set.hpp"
#include "cpp-common/bt2/trace-ir.hpp"

#include "catch2/catch_test_macros.hpp"
#include "common.hpp"
#include "utils/run-in.hpp"

namespace {

/*
 * Builds a trace IR with one event class whose payload field class is
 * the structure field class which `buildFn` returns, and then runs
 * `testFunc` against the payload field of a single instantiated event.
 */
void runWithStructField(std::function<bt2::StructureFieldClass::Shared(bt2::TraceClass)> buildFunc,
                        std::function<void(bt2::StructureField)> testFunc,
                        const std::uint64_t mipVersion = 0)
{
    class Impl final : public RunIn
    {
    public:
        explicit Impl(decltype(buildFunc) implBuildFunc, decltype(testFunc) implTestFunc)
            : _mBuildFunc {std::move(implBuildFunc)},
              _mTestFunc {std::move(implTestFunc)}
        {
        }

        void onMsgIterInit(const bt2::SelfMessageIterator self) override
        {
            const auto traceCls = self.component().createTraceClass();
            const auto streamCls = traceCls->createStreamClass();
            const auto eventCls = streamCls->createEventClass();

            eventCls->payloadFieldClass(*_mBuildFunc(*traceCls));
            _mTestFunc(*self.createEventMessage(*eventCls,
                                                *streamCls->instantiate(*traceCls->instantiate()))
                            ->event()
                            .payloadField());
        }

    private:
        decltype(buildFunc) _mBuildFunc;
        decltype(testFunc) _mTestFunc;
    };

    Impl impl {std::move(buildFunc), std::move(testFunc)};

    runIn(impl, mipVersion);
}

/*
 * Wraps runWithStructField() for the common case of a single payload
 * member field of type `FieldT` built from `classFunc`.
 */
template <typename FieldT>
void runWithField(std::function<bt2::FieldClass::Shared(bt2::TraceClass)> classFunc,
                  std::function<void(FieldT)> testFunc, const std::uint64_t mipVersion = 0)
{
    runWithStructField(
        [classFunc = std::move(classFunc)](const bt2::TraceClass tc) {
            auto pc = tc.createStructureFieldClass();

            pc->appendMember("f", *classFunc(tc));
            return pc;
        },
        [testFunc = std::move(testFunc)](const bt2::StructureField structField) {
            testFunc((*structField["f"]).template as<FieldT>());
        },
        mipVersion);
}

bt2::FieldClass::Shared createBoolFc(const bt2::TraceClass tc)
{
    return tc.createBoolFieldClass();
}

bt2::FieldClass::Shared createBitArrayFc(const bt2::TraceClass tc, const std::uint64_t length = 8)
{
    return tc.createBitArrayFieldClass(length);
}

bt2::FieldClass::Shared createUIntFc(const bt2::TraceClass tc)
{
    return tc.createUnsignedIntegerFieldClass();
}

bt2::FieldClass::Shared createSIntFc(const bt2::TraceClass tc)
{
    return tc.createSignedIntegerFieldClass();
}

bt2::FieldClass::Shared createSinglePrecisionRealFc(const bt2::TraceClass tc)
{
    return tc.createSinglePrecisionRealFieldClass();
}

bt2::FieldClass::Shared createDoublePrecisionRealFc(const bt2::TraceClass tc)
{
    return tc.createDoublePrecisionRealFieldClass();
}

bt2::FieldClass::Shared createStringFc(const bt2::TraceClass tc)
{
    return tc.createStringFieldClass();
}

} /* namespace */

TEST_CASE("Bool field: set true")
{
    runWithField<bt2::BoolField>(createBoolFc, [](const bt2::BoolField field) {
        field.value(true);
        CHECK(field.value() == true);
        CHECK(*field == true);
    });
}

TEST_CASE("Bool field: set false after true")
{
    runWithField<bt2::BoolField>(createBoolFc, [](const bt2::BoolField field) {
        field.value(true);
        field.value(false);
        CHECK(field.value() == false);
    });
}

TEST_CASE("Bool field: assignment via raw value proxy")
{
    runWithField<bt2::BoolField>(createBoolFc, [](const bt2::BoolField field) {
        *field = true;
        CHECK(field.value() == true);
    });
}

TEST_CASE("Bit array field: set/get as integer")
{
    runWithField<bt2::BitArrayField>(
        [](const bt2::TraceClass tc) {
            return createBitArrayFc(tc, 16);
        },
        [](const bt2::BitArrayField field) {
            field.valueAsInteger(0xa5a5);
            CHECK(field.valueAsInteger() == 0xa5a5);
        });
}

TEST_CASE("Bit array field: bitValue() reads each bit")
{
    runWithField<bt2::BitArrayField>(
        [](const bt2::TraceClass tc) {
            return createBitArrayFc(tc, 8);
        },
        [](const bt2::BitArrayField field) {
            field.valueAsInteger(0b10100101);
            CHECK(field.bitValue(0) == true);
            CHECK(field.bitValue(1) == false);
            CHECK(field.bitValue(2) == true);
            CHECK(field.bitValue(3) == false);
            CHECK(field.bitValue(4) == false);
            CHECK(field.bitValue(5) == true);
            CHECK(field.bitValue(6) == false);
            CHECK(field.bitValue(7) == true);
        });
}

TEST_CASE("Bit array field: full-width 64-bit value")
{
    runWithField<bt2::BitArrayField>(
        [](const bt2::TraceClass tc) {
            return createBitArrayFc(tc, 64);
        },
        [](const bt2::BitArrayField field) {
            static constexpr auto val = std::numeric_limits<std::uint64_t>::max();

            field.valueAsInteger(val);
            CHECK(field.valueAsInteger() == val);
        });
}

TEST_CASE("Bit array field: active flag labels with no flags")
{
    runWithField<bt2::BitArrayField>(
        [](const bt2::TraceClass tc) {
            return createBitArrayFc(tc, 8);
        },
        [](const bt2::BitArrayField field) {
            field.valueAsInteger(0xff);
            CHECK(field.activeFlagLabels().length() == 0);
        },
        1);
}

TEST_CASE("Bit array field: active flag labels match value")
{
    runWithField<bt2::BitArrayField>(
        [](const bt2::TraceClass tc) {
            const auto fc = tc.createBitArrayFieldClass(8);

            fc->addFlag("low", bt2::UnsignedIntegerRangeSet::create()->addRange(0, 0));
            fc->addFlag("hi", bt2::UnsignedIntegerRangeSet::create()->addRange(7, 7));
            return fc;
        },
        [](const bt2::BitArrayField field) {
            field.valueAsInteger(0b10000001);

            const auto labels = field.activeFlagLabels();

            REQUIRE(labels.length() == 2);

            const auto l0 = labels[0];
            const auto l1 = labels[1];

            CHECK(((l0 == "low" && l1 == "hi") || (l0 == "hi" && l1 == "low")));
        },
        1);
}

TEST_CASE("Unsigned integer field: zero round-trips")
{
    runWithField<bt2::UnsignedIntegerField>(createUIntFc,
                                            [](const bt2::UnsignedIntegerField field) {
                                                field.value(0);
                                                CHECK(field.value() == 0);
                                            });
}

TEST_CASE("Unsigned integer field: set/get arbitrary value")
{
    runWithField<bt2::UnsignedIntegerField>(createUIntFc,
                                            [](const bt2::UnsignedIntegerField field) {
                                                field.value(123456789);
                                                CHECK(field.value() == 123456789);
                                                CHECK(*field == 123456789);
                                            });
}

TEST_CASE("Unsigned integer field: max 64-bit value")
{
    runWithField<bt2::UnsignedIntegerField>(
        createUIntFc, [](const bt2::UnsignedIntegerField field) {
            static constexpr auto val = std::numeric_limits<std::uint64_t>::max();

            field.value(val);
            CHECK(field.value() == val);
        });
}

TEST_CASE("Unsigned integer field: overwrite previous value")
{
    runWithField<bt2::UnsignedIntegerField>(createUIntFc,
                                            [](const bt2::UnsignedIntegerField field) {
                                                field.value(42);
                                                field.value(7);
                                                CHECK(field.value() == 7);
                                            });
}

TEST_CASE("Signed integer field: zero round-trips")
{
    runWithField<bt2::SignedIntegerField>(createSIntFc, [](const bt2::SignedIntegerField field) {
        field.value(0);
        CHECK(field.value() == 0);
    });
}

TEST_CASE("Signed integer field: positive value")
{
    runWithField<bt2::SignedIntegerField>(createSIntFc, [](const bt2::SignedIntegerField field) {
        field.value(123);
        CHECK(field.value() == 123);
    });
}

TEST_CASE("Signed integer field: negative value")
{
    runWithField<bt2::SignedIntegerField>(createSIntFc, [](const bt2::SignedIntegerField field) {
        field.value(-123);
        CHECK(field.value() == -123);
    });
}

TEST_CASE("Signed integer field: minimum/maximum 64-bit values")
{
    runWithField<bt2::SignedIntegerField>(createSIntFc, [](const bt2::SignedIntegerField field) {
        static constexpr auto maxVal = std::numeric_limits<std::int64_t>::max();
        static constexpr auto minVal = std::numeric_limits<std::int64_t>::min();

        field.value(maxVal);
        CHECK(field.value() == maxVal);
        field.value(minVal);
        CHECK(field.value() == minVal);
    });
}

TEST_CASE("Unsigned enumeration field: get labels for matching value")
{
    runWithField<bt2::UnsignedEnumerationField>(
        [](const bt2::TraceClass tc) {
            const auto fc = tc.createUnsignedEnumerationFieldClass();

            fc->addMapping("zero", bt2::UnsignedIntegerRangeSet::create()->addRange(0, 0));
            fc->addMapping("one-to-three", bt2::UnsignedIntegerRangeSet::create()->addRange(1, 3));
            fc->addMapping("also-three", bt2::UnsignedIntegerRangeSet::create()->addRange(3, 3));
            return fc;
        },
        [](const bt2::UnsignedEnumerationField field) {
            field.value(3);
            CHECK(field.value() == 3);

            const auto labels = field.labels();

            CHECK(labels.length() == 2);
        });
}

TEST_CASE("Unsigned enumeration field: no labels for unmapped value")
{
    runWithField<bt2::UnsignedEnumerationField>(
        [](const bt2::TraceClass tc) {
            const auto fc = tc.createUnsignedEnumerationFieldClass();

            fc->addMapping("a", bt2::UnsignedIntegerRangeSet::create()->addRange(0, 0));
            return fc;
        },
        [](const bt2::UnsignedEnumerationField field) {
            field.value(99);
            CHECK(field.labels().length() == 0);
        });
}

TEST_CASE("Signed enumeration field: get labels for matching value")
{
    runWithField<bt2::SignedEnumerationField>(
        [](const bt2::TraceClass tc) {
            const auto fc = tc.createSignedEnumerationFieldClass();

            fc->addMapping("neg", bt2::SignedIntegerRangeSet::create()->addRange(-10, -1));
            fc->addMapping("zero", bt2::SignedIntegerRangeSet::create()->addRange(0, 0));
            fc->addMapping("pos", bt2::SignedIntegerRangeSet::create()->addRange(1, 10));
            return fc;
        },
        [](const bt2::SignedEnumerationField field) {
            field.value(-5);
            REQUIRE(field.mappingLabels().length() == 1);
            CHECK(field.mappingLabels()[0] == "neg");
            field.value(0);
            REQUIRE(field.mappingLabels().length() == 1);
            CHECK(field.mappingLabels()[0] == "zero");
            field.value(7);
            REQUIRE(field.mappingLabels().length() == 1);
            CHECK(field.mappingLabels()[0] == "pos");
        });
}

TEST_CASE("Signed enumeration field: no labels for unmapped value")
{
    runWithField<bt2::SignedEnumerationField>(
        [](const bt2::TraceClass tc) {
            const auto fc = tc.createSignedEnumerationFieldClass();

            fc->addMapping("only", bt2::SignedIntegerRangeSet::create()->addRange(0, 0));
            return fc;
        },
        [](const bt2::SignedEnumerationField field) {
            field.value(-99);
            CHECK(field.mappingLabels().length() == 0);
        });
}

TEST_CASE("Single-precision real field: zero round-trips")
{
    runWithField<bt2::SinglePrecisionRealField>(createSinglePrecisionRealFc,
                                                [](const bt2::SinglePrecisionRealField field) {
                                                    field.value(0.0f);
                                                    CHECK(field.value() == 0.0f);
                                                });
}

TEST_CASE("Single-precision real field: round trip")
{
    runWithField<bt2::SinglePrecisionRealField>(createSinglePrecisionRealFc,
                                                [](const bt2::SinglePrecisionRealField field) {
                                                    field.value(3.5f);
                                                    CHECK(field.value() == 3.5f);
                                                    field.value(-1234.5f);
                                                    CHECK(field.value() == -1234.5f);
                                                });
}

TEST_CASE("Single-precision real field: infinity")
{
    runWithField<bt2::SinglePrecisionRealField>(
        createSinglePrecisionRealFc, [](const bt2::SinglePrecisionRealField field) {
            constexpr auto inf = std::numeric_limits<float>::infinity();

            field.value(inf);
            CHECK(field.value() == inf);
            field.value(-inf);
            CHECK(field.value() == -inf);
        });
}

TEST_CASE("Single-precision real field: NaN")
{
    runWithField<bt2::SinglePrecisionRealField>(
        createSinglePrecisionRealFc, [](const bt2::SinglePrecisionRealField field) {
            field.value(std::numeric_limits<float>::quiet_NaN());
            CHECK(std::isnan(field.value()));
        });
}

TEST_CASE("Double-precision real field: zero round-trips")
{
    runWithField<bt2::DoublePrecisionRealField>(createDoublePrecisionRealFc,
                                                [](const bt2::DoublePrecisionRealField field) {
                                                    field.value(0.0);
                                                    CHECK(field.value() == 0.0);
                                                });
}

TEST_CASE("Double-precision real field: round trip")
{
    runWithField<bt2::DoublePrecisionRealField>(createDoublePrecisionRealFc,
                                                [](const bt2::DoublePrecisionRealField field) {
                                                    field.value(2.71828);
                                                    CHECK(field.value() == 2.71828);
                                                    field.value(-1.61803398875);
                                                    CHECK(field.value() == -1.61803398875);
                                                });
}

TEST_CASE("Double-precision real field: infinity")
{
    runWithField<bt2::DoublePrecisionRealField>(
        createDoublePrecisionRealFc, [](const bt2::DoublePrecisionRealField field) {
            constexpr auto inf = std::numeric_limits<double>::infinity();

            field.value(inf);
            CHECK(field.value() == inf);
        });
}

TEST_CASE("Double-precision real field: NaN")
{
    runWithField<bt2::DoublePrecisionRealField>(
        createDoublePrecisionRealFc, [](const bt2::DoublePrecisionRealField field) {
            constexpr auto nan = std::numeric_limits<double>::quiet_NaN();

            field.value(nan);
            CHECK(std::isnan(field.value()));
        });
}

TEST_CASE("String field: set value")
{
    runWithField<bt2::StringField>(createStringFc, [](const bt2::StringField field) {
        *field = "pomme";
        CHECK(field.value() == "pomme");
        CHECK(field.length() == 5);
    });
}

TEST_CASE("String field: clear() resets value and length")
{
    runWithField<bt2::StringField>(createStringFc, [](const bt2::StringField field) {
        *field = "pomme";
        field.clear();
        CHECK(field.value() == "");
        CHECK(field.length() == 0);
    });
}

TEST_CASE("String field: append C string")
{
    runWithField<bt2::StringField>(createStringFc, [](const bt2::StringField field) {
        *field = "pom";
        field.append(bt2c::CStringView {"me"});
        CHECK(field.value() == "pomme");
        CHECK(field.length() == 5);
    });
}

TEST_CASE("String field: append with explicit length")
{
    runWithField<bt2::StringField>(createStringFc, [](const bt2::StringField field) {
        *field = "pom";
        field.append(bt2c::CStringView {"meXX"}, 2);
        CHECK(field.value() == "pomme");
    });
}

TEST_CASE("String field: append `std::string`")
{
    runWithField<bt2::StringField>(createStringFc, [](const bt2::StringField field) {
        *field = "pom";
        field.append(std::string {"me"});
        CHECK(field.value() == "pomme");
    });
}

TEST_CASE("String field: assignment overwrites previous value")
{
    runWithField<bt2::StringField>(createStringFc, [](const bt2::StringField field) {
        *field = "first";
        *field = "second";
        CHECK(field.value() == "second");
    });
}

TEST_CASE("String field: append to empty equals set")
{
    runWithField<bt2::StringField>(createStringFc, [](const bt2::StringField field) {
        field.append(bt2c::CStringView {"yo"});
        CHECK(field.value() == "yo");
    });
}

TEST_CASE("Structure field: length matches member count")
{
    runWithField<bt2::StructureField>(
        [](const bt2::TraceClass tc) {
            auto fc = tc.createStructureFieldClass();

            fc->appendMember("a", *tc.createUnsignedIntegerFieldClass());
            fc->appendMember("b", *tc.createStringFieldClass());
            fc->appendMember("c", *tc.createBoolFieldClass());
            return fc;
        },
        [](const bt2::StructureField field) {
            CHECK(field.length() == 3);
        });
}

TEST_CASE("Structure field: member access by index")
{
    runWithField<bt2::StructureField>(
        [](const bt2::TraceClass tc) {
            auto fc = tc.createStructureFieldClass();

            fc->appendMember("count", *tc.createUnsignedIntegerFieldClass());
            fc->appendMember("name", *tc.createStringFieldClass());
            return fc;
        },
        [](const bt2::StructureField field) {
            field[0].asUnsignedInteger().value(42);
            *field[1].asString() = "alice";
            REQUIRE(field.length() == 2);
            CHECK(field[0].asUnsignedInteger().value() == 42);
            CHECK(field[1].asString().value() == "alice");
        });
}

TEST_CASE("Structure field: member access by name")
{
    runWithField<bt2::StructureField>(
        [](const bt2::TraceClass tc) {
            auto fc = tc.createStructureFieldClass();

            fc->appendMember("count", *tc.createUnsignedIntegerFieldClass());
            fc->appendMember("name", *tc.createStringFieldClass());
            return fc;
        },
        [](const bt2::StructureField field) {
            (*field["count"]).asUnsignedInteger().value(42);
            *(*field["name"]).asString() = "alice";

            CHECK((*field["count"]).asUnsignedInteger().value() == 42);
            CHECK((*field["name"]).asString().value() == "alice");
        });
}

TEST_CASE("Structure field: lookup of unknown member returns empty optional")
{
    runWithField<bt2::StructureField>(
        [](const bt2::TraceClass tc) {
            auto fc = tc.createStructureFieldClass();

            fc->appendMember("only", *tc.createBoolFieldClass());
            return fc;
        },
        [](const bt2::StructureField field) {
            CHECK(field["nope"].hasObject() == false);
            CHECK(field["only"].hasObject() == true);
        });
}

TEST_CASE("Static array field: length is fixed")
{
    runWithField<bt2::ArrayField>(
        [](const bt2::TraceClass tc) {
            return tc.createStaticArrayFieldClass(*tc.createUnsignedIntegerFieldClass(), 4);
        },
        [](const bt2::ArrayField field) {
            CHECK(field.length() == 4);
        });
}

TEST_CASE("Static array field: element access")
{
    runWithField<bt2::ArrayField>(
        [](const bt2::TraceClass tc) {
            return tc.createStaticArrayFieldClass(*tc.createUnsignedIntegerFieldClass(), 3);
        },
        [](const bt2::ArrayField field) {
            REQUIRE(field.length() == 3);
            field[0].asUnsignedInteger().value(10);
            field[1].asUnsignedInteger().value(20);
            field[2].asUnsignedInteger().value(30);
            CHECK(field[0].asUnsignedInteger().value() == 10);
            CHECK(field[1].asUnsignedInteger().value() == 20);
            CHECK(field[2].asUnsignedInteger().value() == 30);
        });
}

TEST_CASE("Static array field: zero length is allowed")
{
    runWithField<bt2::ArrayField>(
        [](const bt2::TraceClass tc) {
            return tc.createStaticArrayFieldClass(*tc.createUnsignedIntegerFieldClass(), 0);
        },
        [](const bt2::ArrayField field) {
            CHECK(field.length() == 0);
        });
}

TEST_CASE("Dynamic array field: initial length is zero")
{
    runWithField<bt2::DynamicArrayField>(
        [](const bt2::TraceClass tc) {
            return tc.createDynamicArrayFieldClass(*tc.createSignedIntegerFieldClass());
        },
        [](const bt2::DynamicArrayField field) {
            CHECK(field.length() == 0);
        });
}

TEST_CASE("Dynamic array field: set length grows array")
{
    runWithField<bt2::DynamicArrayField>(
        [](const bt2::TraceClass tc) {
            return tc.createDynamicArrayFieldClass(*tc.createSignedIntegerFieldClass());
        },
        [](const bt2::DynamicArrayField field) {
            field.length(5);
            REQUIRE(field.length() == 5);

            for (std::uint64_t i = 0; i < 5; ++i) {
                field[i].asSignedInteger().value(static_cast<std::int64_t>(i) - 2);
            }

            for (std::uint64_t i = 0; i < 5; ++i) {
                CHECK(field[i].asSignedInteger().value() == static_cast<std::int64_t>(i) - 2);
            }
        });
}

TEST_CASE("Dynamic array field: set length to zero")
{
    runWithField<bt2::DynamicArrayField>(
        [](const bt2::TraceClass tc) {
            return tc.createDynamicArrayFieldClass(*tc.createBoolFieldClass());
        },
        [](const bt2::DynamicArrayField field) {
            field.length(3);
            field.length(0);
            CHECK(field.length() == 0);
        });
}

TEST_CASE("Option field: `hasField(true)` makes inner field accessible")
{
    runWithField<bt2::OptionField>(
        [](const bt2::TraceClass tc) {
            return tc.createOptionFieldClass(*tc.createUnsignedIntegerFieldClass());
        },
        [](const bt2::OptionField field) {
            field.hasField(true);
            CHECK(field.hasField() == true);
            REQUIRE(field.field().hasObject());
            (*field.field()).asUnsignedInteger().value(7);
            CHECK((*field.field()).asUnsignedInteger().value() == 7);
        });
}

TEST_CASE("Option field: `hasField(false)` hides inner field")
{
    runWithField<bt2::OptionField>(
        [](const bt2::TraceClass tc) {
            return tc.createOptionFieldClass(*tc.createUnsignedIntegerFieldClass());
        },
        [](const bt2::OptionField field) {
            field.hasField(true);
            field.hasField(false);
            CHECK(field.hasField() == false);
            CHECK(field.field().hasObject() == false);
        });
}

TEST_CASE("Option field (bool selector): hasField() independent of selector")
{
    runWithStructField(
        [](const bt2::TraceClass tc) {
            const auto selFc = tc.createBoolFieldClass();
            const auto optFc = tc.createOptionWithBoolSelectorFieldClass(
                *tc.createSignedIntegerFieldClass(), *selFc);
            auto pc = tc.createStructureFieldClass();

            pc->appendMember("sel", *selFc);
            pc->appendMember("opt", *optFc);
            return pc;
        },
        [](const bt2::StructureField sf) {
            (*sf["sel"]).asBool().value(true);

            const auto opt = (*sf["opt"]).asOption();

            opt.hasField(true);
            CHECK(opt.hasField() == true);
            (*opt.field()).asSignedInteger().value(-99);
            CHECK((*opt.field()).asSignedInteger().value() == -99);
        });
}

TEST_CASE("Variant field (no selector): selectOption() picks the option")
{
    runWithField<bt2::VariantField>(
        [](const bt2::TraceClass tc) {
            const auto vfc = tc.createVariantFieldClass();

            vfc->appendOption("u", *tc.createUnsignedIntegerFieldClass());
            vfc->appendOption("s", *tc.createStringFieldClass());
            vfc->appendOption("b", *tc.createBoolFieldClass());
            return vfc;
        },
        [](const bt2::VariantField field) {
            field.selectOption(1);
            CHECK(field.selectedOptionIndex() == 1);
            *field.selectedOptionField().asString() = "hi";
            CHECK(field.selectedOptionField().asString().value() == "hi");
        });
}

TEST_CASE("Variant field (no selector): switching options")
{
    runWithField<bt2::VariantField>(
        [](const bt2::TraceClass tc) {
            const auto fc = tc.createVariantFieldClass();

            fc->appendOption("u", *tc.createUnsignedIntegerFieldClass());
            fc->appendOption("s", *tc.createSignedIntegerFieldClass());
            return fc;
        },
        [](const bt2::VariantField field) {
            field.selectOption(0);
            field.selectedOptionField().asUnsignedInteger().value(5);
            field.selectOption(1);
            field.selectedOptionField().asSignedInteger().value(-5);
            CHECK(field.selectedOptionIndex() == 1);
            CHECK(field.selectedOptionField().asSignedInteger().value() == -5);
        });
}

TEST_CASE("Variant field (unsigned integer selector): selectOption() picks the option")
{
    runWithStructField(
        [](const bt2::TraceClass tc) {
            const auto selFc = tc.createUnsignedIntegerFieldClass();
            const auto variantFc = tc.createVariantWithUnsignedIntegerSelectorFieldClass(*selFc);

            variantFc->appendOption("u", *tc.createUnsignedIntegerFieldClass(),
                                    bt2::UnsignedIntegerRangeSet::create()->addRange(0, 0));
            variantFc->appendOption("s", *tc.createSignedIntegerFieldClass(),
                                    bt2::UnsignedIntegerRangeSet::create()->addRange(1, 1));

            auto fc = tc.createStructureFieldClass();

            fc->appendMember("sel", *selFc);
            fc->appendMember("var", *variantFc);
            return fc;
        },
        [](const bt2::StructureField structField) {
            (*structField["sel"]).asUnsignedInteger().value(1);

            const auto variantField = (*structField["var"]).asVariant();

            variantField.selectOption(1);
            CHECK(variantField.selectedOptionIndex() == 1);
            variantField.selectedOptionField().asSignedInteger().value(-42);
            CHECK(variantField.selectedOptionField().asSignedInteger().value() == -42);
        });
}

TEST_CASE("Variant-with-signed-int-selector field: selectOption picks the option")
{
    runWithStructField(
        [](const bt2::TraceClass tc) {
            const auto selFc = tc.createSignedIntegerFieldClass();
            const auto variantFc = tc.createVariantWithSignedIntegerSelectorFieldClass(*selFc);

            variantFc->appendOption("neg", *tc.createSignedIntegerFieldClass(),
                                    bt2::SignedIntegerRangeSet::create()->addRange(-10, -1));
            variantFc->appendOption("pos", *tc.createUnsignedIntegerFieldClass(),
                                    bt2::SignedIntegerRangeSet::create()->addRange(1, 10));

            auto fc = tc.createStructureFieldClass();

            fc->appendMember("sel", *selFc);
            fc->appendMember("var", *variantFc);
            return fc;
        },
        [](const bt2::StructureField structField) {
            (*structField["sel"]).asSignedInteger().value(-3);

            const auto variantField = (*structField["var"]).asVariant();

            variantField.selectOption(0);
            CHECK(variantField.selectedOptionIndex() == 0);
            variantField.selectedOptionField().asSignedInteger().value(-7);
            CHECK(variantField.selectedOptionField().asSignedInteger().value() == -7);
        });
}

TEST_CASE("Static BLOB field: length is fixed")
{
    runWithField<bt2::BlobField>(
        [](const bt2::TraceClass tc) {
            return tc.createStaticBlobFieldClass(8);
        },
        [](const bt2::BlobField field) {
            CHECK(field.length() == 8);
        },
        1);
}

TEST_CASE("Static BLOB field: write and read bytes")
{
    runWithField<bt2::BlobField>(
        [](const bt2::TraceClass tc) {
            return tc.createStaticBlobFieldClass(4);
        },
        [](const bt2::BlobField field) {
            const auto data = field.data();

            REQUIRE(data.size() == 4);
            data[0] = 0x12;
            data[1] = 0x34;
            data[2] = 0x56;
            data[3] = 0x78;
            CHECK(intFromByte(field.data()[0]) == 0x12);
            CHECK(intFromByte(field.data()[1]) == 0x34);
            CHECK(intFromByte(field.data()[2]) == 0x56);
            CHECK(intFromByte(field.data()[3]) == 0x78);
        },
        1);
}

TEST_CASE("Dynamic BLOB field: initial length is zero")
{
    runWithField<bt2::DynamicBlobField>(
        [](const bt2::TraceClass tc) {
            return tc.createDynamicBlobWithoutLengthFieldLocationFieldClass();
        },
        [](const bt2::DynamicBlobField field) {
            CHECK(field.length() == 0);
        },
        1);
}

TEST_CASE("Dynamic BLOB field: set length and write bytes")
{
    runWithField<bt2::DynamicBlobField>(
        [](const bt2::TraceClass tc) {
            return tc.createDynamicBlobWithoutLengthFieldLocationFieldClass();
        },
        [](const bt2::DynamicBlobField field) {
            field.length(3);
            REQUIRE(field.length() == 3);

            const auto data = field.data();

            data[0] = 0xab;
            data[1] = 0xcd;
            data[2] = 0xef;
            CHECK(intFromByte(field.data()[0]) == 0xab);
            CHECK(intFromByte(field.data()[1]) == 0xcd);
            CHECK(intFromByte(field.data()[2]) == 0xef);
        },
        1);
}

TEST_CASE("Dynamic BLOB field: `length(0)` clears")
{
    runWithField<bt2::DynamicBlobField>(
        [](const bt2::TraceClass tc) {
            return tc.createDynamicBlobWithoutLengthFieldLocationFieldClass();
        },
        [](const bt2::DynamicBlobField field) {
            field.length(5);
            field.length(0);
            CHECK(field.length() == 0);
        },
        1);
}
