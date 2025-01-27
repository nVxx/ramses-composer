/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <gtest/gtest.h>

#include "RamsesBaseFixture.h"
#include "ramses_adaptor/PerspectiveCameraAdaptor.h"
#include "user_types/PerspectiveCamera.h"

using namespace raco;
using raco::core::ValueHandle;
using raco::user_types::LuaScript;
using raco::user_types::PerspectiveCamera;

class PerspectiveCameraAdaptorTest : public RamsesBaseFixture<> {};

TEST_F(PerspectiveCameraAdaptorTest, quaternion_link_camera_still_active) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface(IN,OUT)
	IN.in_value = Type:Vec4f()
	OUT.out_value = Type:Vec4f()
end

function run(IN,OUT)
	OUT.out_value = IN.in_value
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"inputs", "in_value", "x"}}, 5.0f);
	dispatch();

	auto camera = commandInterface.createObject(PerspectiveCamera::typeDescription.typeName, "Camera");
	dispatch();

	commandInterface.addLink({luaScript, {"outputs", "out_value"}}, {camera, {"rotation"}});
	dispatch();

	commandInterface.set({camera, {"frustum", "aspectRatio"}}, 7.0);
	dispatch();

	auto cameras{select<ramses::PerspectiveCamera>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_PerspectiveCamera)};
	ASSERT_EQ(cameras.size(), 1);
	ASSERT_FLOAT_EQ(cameras.front()->getAspectRatio(), 7.0F);
}

TEST_F(PerspectiveCameraAdaptorTest, bindingID) {
	auto camera = commandInterface.createObject(PerspectiveCamera::typeDescription.typeName, "Camera");
	dispatch();

	auto cameras{select<rlogic::RamsesCameraBinding>(sceneContext.logicEngine(), "Camera_CameraBinding")};
	ASSERT_EQ(cameras->getUserId(), camera->objectIDAsRamsesLogicID());
}

TEST_F(PerspectiveCameraAdaptorTest, bindingID_name_change) {
	auto camera = commandInterface.createObject(PerspectiveCamera::typeDescription.typeName, "Camera");
	dispatch();

	commandInterface.set({camera, &PerspectiveCamera::objectName_}, std::string("Changed"));
	dispatch();

	auto cameras{select<rlogic::RamsesCameraBinding>(sceneContext.logicEngine(), "Changed_CameraBinding")};
	ASSERT_EQ(cameras->getUserId(), camera->objectIDAsRamsesLogicID());
}