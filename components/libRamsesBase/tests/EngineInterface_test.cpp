/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "RamsesBaseFixture.h"
#include <gtest/gtest.h>

using namespace raco::ramses_base;
using raco::core::EnginePrimitive;

class EngineInterfaceTest : public RamsesBaseFixture<> {};

TEST_F(EngineInterfaceTest, parseLuaScript_struct) {
	const std::string script = R"(
function interface(IN,OUT)
	IN.struct = {
        a = Type:Float(),
        b = Type:Float()
    }
end

function run(IN,OUT)
end
)";
	std::string error;
	raco::core::PropertyInterfaceList in;
	raco::core::PropertyInterfaceList out;
	raco::data_storage::Table modules;
	backend.coreInterface()->parseLuaScript(script, "myScript", {}, modules, in, out, error);

	EXPECT_EQ(1, in.size());
	const auto& structProperty = in.at(0);
	EXPECT_EQ("struct", structProperty.name);
	EXPECT_EQ(EnginePrimitive::Struct, structProperty.type);
	EXPECT_EQ(2, structProperty.children.size());

	const auto& a{structProperty.children.at(0)};
	EXPECT_EQ("a", a.name);
	EXPECT_EQ(EnginePrimitive::Double, a.type);
	EXPECT_EQ(0, a.children.size());

	const auto& b{structProperty.children.at(1)};
	EXPECT_EQ("b", b.name);
	EXPECT_EQ(EnginePrimitive::Double, b.type);
	EXPECT_EQ(0, b.children.size());
}

TEST_F(EngineInterfaceTest, parseLuaScript_arrayNT) {
	const std::string script = R"(
function interface(IN,OUT)
	IN.vec = Type:Array(5, Type:Float())
end

function run(IN,OUT)
end
)";
	std::string error;
	raco::core::PropertyInterfaceList in;
	raco::core::PropertyInterfaceList out;
	raco::data_storage::Table modules;
	backend.coreInterface()->parseLuaScript(script, "myScript", {}, modules, in, out, error);

	EXPECT_EQ(1, in.size());
	EXPECT_EQ(EnginePrimitive::Array, in.at(0).type);
	EXPECT_EQ(5, in.at(0).children.size());
	for (size_t i{0}; i < 5; i++) {
		EXPECT_EQ(EnginePrimitive::Double, in.at(0).children.at(i).type);
	}
}
