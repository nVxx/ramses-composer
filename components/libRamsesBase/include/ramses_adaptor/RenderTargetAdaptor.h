/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "ramses_adaptor/ObjectAdaptor.h"
#include "user_types/RenderTarget.h"
#include <ramses-client-api/RenderTarget.h>

namespace raco::ramses_adaptor {

class RenderTargetAdaptor : public TypedObjectAdaptor<user_types::RenderTarget, ramses::RenderTarget> {
public:
	explicit RenderTargetAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::RenderTarget> editorObject);

	bool sync(core::Errors* errors) override;
	std::vector<ExportInformation> getExportInformation() const override;

private:
	template <typename RenderBufferAdaptorClass>
	bool collectBuffers(std::vector<ramses_base::RamsesRenderBuffer>& buffers, const std::initializer_list<raco::core::SEditorObject>& userTypeBuffers, ramses::RenderTargetDescription& rtDesc, core::Errors* errors);

	std::array<components::Subscription, 16> subscriptions_;
};

};	// namespace raco::ramses_adaptor
