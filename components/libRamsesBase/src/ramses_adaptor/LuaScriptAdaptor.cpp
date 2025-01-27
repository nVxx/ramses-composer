/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/LuaScriptAdaptor.h"

#include "ramses_adaptor/LuaScriptModuleAdaptor.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/utilities.h"
#include "ramses_base/LogicEngineFormatter.h"
#include "ramses_base/Utils.h"
#include "user_types/PrefabInstance.h"
#include "utils/FileUtils.h"

namespace raco::ramses_adaptor {

LuaScriptAdaptor::LuaScriptAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::LuaScript> editorObject)
	: UserTypeObjectAdaptor{sceneAdaptor, editorObject},
	  nameSubscription_{sceneAdaptor_->dispatcher()->registerOn({editorObject_, &user_types::LuaScript::objectName_}, [this]() {
		  tagDirty();
		  recreateStatus_ = true;
	  })},
	  subscription_{sceneAdaptor_->dispatcher()->registerOnPreviewDirty(editorObject_, [this]() {
		  setupInputValuesSubscription();
		  tagDirty();
		  recreateStatus_ = true;
	  })},
	  childrenSubscription_(sceneAdaptor_->dispatcher()->registerOnPropertyChange("children", [this](core::ValueHandle handle) {
		  if (parent_ != editorObject_->getParent()) {
			  setupParentSubscription();
			  tagDirty();
			  recreateStatus_ = true;
		  }
	  })),
	  stdModuleSubscription_{sceneAdaptor_->dispatcher()->registerOnChildren({editorObject_, &user_types::LuaScript::stdModules_}, [this](auto) {
		  tagDirty();
		  recreateStatus_ = true;
	  })},
	  moduleSubscription_{sceneAdaptor_->dispatcher()->registerOnChildren({editorObject_, &user_types::LuaScript::luaModules_}, [this](auto) {
		  tagDirty();
		  recreateStatus_ = true;
	  })} {
	setupParentSubscription();
	setupInputValuesSubscription();
}

void LuaScriptAdaptor::getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const {
	logicNodes.push_back(rlogicLuaScript());
}

void LuaScriptAdaptor::setupParentSubscription() {
	parent_ = editorObject_->getParent();

	if (parent_ && parent_->as<user_types::PrefabInstance>()) {
		parentNameSubscription_ = sceneAdaptor_->dispatcher()->registerOn({parent_, &user_types::LuaScript::objectName_}, [this]() {
			tagDirty();
			recreateStatus_ = true;
		});
	} else {
		parentNameSubscription_ = components::Subscription{};
	}
}

void LuaScriptAdaptor::setupInputValuesSubscription() {
	inputSubscription_ = sceneAdaptor_->dispatcher()->registerOnChildren({editorObject_, &user_types::LuaScript::inputs_}, [this](auto) {
		// Only normal tag dirty here; don't set recreateStatus_
		tagDirty();
	});
}

std::string LuaScriptAdaptor::generateRamsesObjectName() const {
	std::string ramsesObjectName;

	if (parent_ && parent_->as<user_types::PrefabInstance>()) {
		ramsesObjectName = parent_->objectName() + ".";
	}

	ramsesObjectName += editorObject_->objectName();
	return ramsesObjectName;
}

bool LuaScriptAdaptor::sync(core::Errors* errors) {
	ObjectAdaptor::sync(errors);

	if (recreateStatus_) {
		auto scriptContent = utils::file::read(raco::core::PathQueries::resolveUriPropertyToAbsolutePath(sceneAdaptor_->project(), {editorObject_, &user_types::LuaScript::uri_}));
		LOG_TRACE(log_system::RAMSES_ADAPTOR, "{}: {}", generateRamsesObjectName(), scriptContent);
		luaScript_.reset();
		if (!scriptContent.empty()) {
			std::vector<raco::ramses_base::RamsesLuaModule> modules;
			auto luaConfig = raco::ramses_base::createLuaConfig(editorObject_->stdModules_->activeModules());
			if (!sceneAdaptor_->optimizeForExport()) {
				luaConfig.enableDebugLogFunctions();
			}
			const auto& moduleDeps = editorObject_->luaModules_.asTable();
			for (auto i = 0; i < moduleDeps.size(); ++i) {
				if (auto moduleRef = moduleDeps.get(i)->asRef()) {
					auto moduleAdaptor = sceneAdaptor_->lookup<LuaScriptModuleAdaptor>(moduleRef);
					if (auto module = moduleAdaptor->module()) {
						modules.emplace_back(module);
						luaConfig.addDependency(moduleDeps.name(i), *module);
					}
				}
			}
			luaScript_ = raco::ramses_base::ramsesLuaScript(&sceneAdaptor_->logicEngine(), scriptContent, luaConfig, modules, generateRamsesObjectName(), editorObject_->objectIDAsRamsesLogicID());
		}
	}

	if (luaScript_) {
		core::ValueHandle luaInputs{editorObject_, &user_types::LuaScript::inputs_};
		auto success = setLuaInputInEngine(luaScript_->getInputs(), luaInputs);
		LOG_WARNING_IF(log_system::RAMSES_ADAPTOR, !success, "Script set properties failed: {}", LogicEngineErrors{sceneAdaptor_->logicEngine()});
	}

	tagDirty(false);
	recreateStatus_ = false;
	return true;
}

void LuaScriptAdaptor::readDataFromEngine(core::DataChangeRecorder &recorder) {
	if (luaScript_) {
		core::ValueHandle luaOutputs{editorObject_, &user_types::LuaScript::outputs_};
		getOutputFromEngine(*luaScript_->getOutputs(), luaOutputs, recorder);
	}
}

const rlogic::Property* LuaScriptAdaptor::getProperty(const std::vector<std::string>& names) {
	if (luaScript_ && names.size() >= 1) {
		const rlogic::Property* prop{names.at(0) == "inputs" ? luaScript_->getInputs() : luaScript_->getOutputs()};
		return ILogicPropertyProvider::getPropertyRecursive(prop, names, 1);
	}	
	return nullptr;
}

void LuaScriptAdaptor::onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) {
	core::ValueHandle const valueHandle{editorObject_};
	if(errors.hasError(valueHandle)) {
		return;
	}
	errors.addError(core::ErrorCategory::RAMSES_LOGIC_RUNTIME, level, valueHandle, message);
}

std::vector<ExportInformation> LuaScriptAdaptor::getExportInformation() const {
	if (luaScript_ == nullptr) {
		return {};
	}

	return {ExportInformation{"LuaScript", luaScript_->getName().data()}};
}

}  // namespace raco::ramses_adaptor
