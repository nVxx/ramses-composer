/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_adaptor/TimerAdaptor.h"

#include "core/Queries.h"

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

TimerAdaptor::TimerAdaptor(SceneAdaptor* sceneAdaptor, raco::user_types::STimer timer)
	: UserTypeObjectAdaptor{sceneAdaptor, timer},
	  timerNode_{raco::ramses_base::ramsesTimer(&sceneAdaptor_->logicEngine(), editorObject_->objectName(), editorObject_->objectIDAsRamsesLogicID())},
	  dirtySubscription_{
		  sceneAdaptor->dispatcher()->registerOnPreviewDirty(editorObject_, [this]() { tagDirty(); })},
	  nameSubscription_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::Timer::objectName_}, [this]() { tagDirty(); })},
	  inputSubscription_{
		  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject_, &user_types::Timer::inputs_}, [this](auto) { tagDirty(); })} {}

void TimerAdaptor::getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const {
	logicNodes.emplace_back(timerNode_.get());
}

const rlogic::Property* TimerAdaptor::getProperty(const std::vector<std::string>& propertyNamesVector) {
	if (timerNode_) {
		const auto& propGroup = propertyNamesVector.front();
		if (propGroup == "inputs") {
			return timerNode_->getInputs()->getChild("ticker_us");
		} else {
			return timerNode_->getOutputs()->getChild("ticker_us");
		}
	}
	return nullptr;
}

void TimerAdaptor::onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) {
	core::ValueHandle const valueHandle{baseEditorObject()};
	if (errors.hasError(valueHandle)) {
		return;
	}
	errors.addError(core::ErrorCategory::RAMSES_LOGIC_RUNTIME, level, valueHandle, message);
}

bool TimerAdaptor::sync(core::Errors* errors) {
	errors->removeError({editorObject_->shared_from_this()});

	if (timerNode_->getName() != editorObject_->objectName()) {
		timerNode_ = raco::ramses_base::ramsesTimer(&sceneAdaptor_->logicEngine(), editorObject_->objectName(), editorObject_->objectIDAsRamsesLogicID());
	}

	timerNode_->getInputs()->getChild("ticker_us")->set(editorObject_->inputs_->get("ticker_us")->asInt64());

	tagDirty(false);
	return true;
}

void TimerAdaptor::readDataFromEngine(core::DataChangeRecorder& recorder) {
	core::ValueHandle animOutputs{editorObject_, {"outputs", "ticker_us"}};
	getOutputFromEngine(*timerNode_->getOutputs()->getChild("ticker_us"), animOutputs, recorder);
}

std::vector<ExportInformation> TimerAdaptor::getExportInformation() const {
	if (timerNode_ == nullptr) {
		return {};
	}

	return {ExportInformation{"Timer", timerNode_->getName().data()}};
}

};	// namespace raco::ramses_adaptor
