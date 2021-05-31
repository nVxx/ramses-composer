/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/MeshNode.h"

#include "user_types/Mesh.h"
#include "user_types/EngineTypeAnnotation.h"

#include "core/Context.h"
#include "core/CoreFormatter.h"
#include "core/Errors.h"
#include "core/Project.h"
#include "user_types/UserObjectFactory.h"

#include <memory>

namespace raco::user_types {

void MeshNode::onBeforeDeleteObject(Errors& errors) const {
	Node::onBeforeDeleteObject(errors);
}

std::vector<std::string> MeshNode::getMaterialNames() {
	SMesh mesh = std::dynamic_pointer_cast<Mesh>(*mesh_);
	if (mesh) {
		return mesh->materialNames();
	}
	return std::vector<std::string>();
}

void MeshNode::createMaterialSlot(std::string const& name) {
	auto container = materials_->addProperty(name, PrimitiveType::Table);
	container->asTable().addProperty("material", new Value<SMaterial>());
	Table& options = container->asTable().addProperty("options", PrimitiveType::Table)->asTable();
	{
		options.addProperty("blendOperationColor", UserObjectFactory::staticCreateProperty<int, DisplayNameAnnotation, EnumerationAnnotation>(DEFAULT_VALUE_MATERIAL_BLEND_OPERATION_COLOR, {"Blend Operation Color"}, {EngineEnumeration::BlendOperation}));
		options.addProperty("blendOperationAlpha", UserObjectFactory::staticCreateProperty<int, DisplayNameAnnotation, EnumerationAnnotation>(DEFAULT_VALUE_MATERIAL_BLEND_OPERATION_ALPHA, {"Blend Operation Alpha"}, {EngineEnumeration::BlendOperation}));
		options.addProperty("blendFactorSrcColor", UserObjectFactory::staticCreateProperty<int, DisplayNameAnnotation, EnumerationAnnotation>(DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_SRC_COLOR, {"Blend Factor Src Color"}, {EngineEnumeration::BlendFactor}));
		options.addProperty("blendFactorDestColor", UserObjectFactory::staticCreateProperty<int, DisplayNameAnnotation, EnumerationAnnotation>(DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_DEST_COLOR, {"Blend Factor Dest Color"}, {EngineEnumeration::BlendFactor}));
		options.addProperty("blendFactorSrcAlpha", UserObjectFactory::staticCreateProperty<int, DisplayNameAnnotation, EnumerationAnnotation>(DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_SRC_ALPHA, {"Blend Factor Src Alpha"}, {EngineEnumeration::BlendFactor}));
		options.addProperty("blendFactorDestAlpha", UserObjectFactory::staticCreateProperty<int, DisplayNameAnnotation, EnumerationAnnotation>(DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_DEST_ALPHA, {"Blend Factor Dest Alpha"}, {EngineEnumeration::BlendFactor}));

		options.addProperty("blendColor", UserObjectFactory::staticCreateProperty<Vec4f, DisplayNameAnnotation>({}, {"Blend Color"}));
		options.addProperty("depthwrite", UserObjectFactory::staticCreateProperty<bool, DisplayNameAnnotation>({true}, {"Depth Write"}));

		options.addProperty("depthfunction", UserObjectFactory::staticCreateProperty<int, DisplayNameAnnotation, EnumerationAnnotation>({DEFAULT_VALUE_MATERIAL_DEPTH_FUNCTION}, {"Depth Function"}, {EngineEnumeration::DepthFunction}));

		options.addProperty("cullmode", UserObjectFactory::staticCreateProperty<int, DisplayNameAnnotation, EnumerationAnnotation>(DEFAULT_VALUE_MATERIAL_CULL_MODE, {"Cull Mode"}, {EngineEnumeration::CullMode}));
	}
	container->asTable().addProperty("uniforms", PrimitiveType::Table);
}

void MeshNode::updateMaterialSlots(BaseContext& context, std::vector<std::string> const& materialNames) {
	for (auto matName : materialNames) {
		if (!materials_->hasProperty(matName)) {
			createMaterialSlot(matName);
		}
	}
	std::vector<std::string> toRemove;
	for (size_t i = 0; i < materials_->size(); i++) {
		if (std::find(materialNames.begin(), materialNames.end(), materials_->name(i)) == materialNames.end()) {
			toRemove.emplace_back(materials_->name(i));
		}
	}
	for (auto name : toRemove) {
		ValueHandle matHandle{shared_from_this(), {"materials"}};
		context.removeProperty(matHandle, materials_.asTable().index(name));
	}

	context.changeMultiplexer().recordValueChanged(ValueHandle(shared_from_this(), {"materials"}));
}


size_t MeshNode::numMaterialSlots() {
	return materials_->size();
}

SMaterial MeshNode::getMaterial(size_t materialSlot) {
	if (materialSlot < materials_->size()) {
		return std::dynamic_pointer_cast<Material>(materials_->get(materialSlot)->asTable().get("material")->asRef());
	}
	return nullptr;
}

Table* MeshNode::getUniformContainer(size_t materialSlot) {
	if (materialSlot < materials_->size()) {
		return &materials_->get(materialSlot)->asTable().get("uniforms")->asTable();
	}
	return nullptr;
};

ValueHandle MeshNode::getMaterialHandle(size_t materialSlot) {
	if (materialSlot < materials_->size()) {
		return ValueHandle(shared_from_this(), {"materials"})[materialSlot].get("material");
	}
	return ValueHandle();
}

ValueHandle MeshNode::getUniformContainerHandle(size_t materialSlot) {
	if (materialSlot < materials_->size()) {
		return ValueHandle(shared_from_this(), {"materials"})[materialSlot].get("uniforms");
	}
	return ValueHandle();
}

ValueHandle MeshNode::getMaterialOptionsHandle(size_t materialSlot) {
	if (materialSlot < materials_->size()) {
		return ValueHandle(shared_from_this(), {"materials"})[materialSlot].get("options");
	}
	return ValueHandle();
}

void MeshNode::updateUniformContainer(BaseContext& context, const std::string& materialName, const Table* src, ValueHandle& destUniforms) {
	if (src) {
		const Table &dest = destUniforms.constValueRef()->asTable();
		std::vector<std::string> toRemove;
		for (size_t i = 0; i < dest.size(); i++) {
			std::string name = dest.name(i);
			if (!src->hasProperty(name)) {
				toRemove.emplace_back(name);
			} else {
				auto srcAnno = src->get(name)->query<EngineTypeAnnotation>();
				assert(srcAnno != nullptr);
				auto destAnno = dest.get(i)->query<EngineTypeAnnotation>();
				assert(destAnno != nullptr);

				if (destAnno->type() != srcAnno->type()) {
					toRemove.emplace_back(name);
				}
			}
		}
		for (auto name : toRemove) {
			const ValueBase* v = dest.get(name);
			auto anno = v->query<EngineTypeAnnotation>();
			EnginePrimitive engineType = anno->type();
			cachedUniformValues_[std::make_tuple(materialName, name, engineType)] = v->clone(nullptr);

			context.removeProperty(destUniforms, dest.index(name));
		}

		for (size_t i = 0; i < src->size(); i++) {
			std::string name = src->name(i);
			if (!dest.hasProperty(name)) {
				auto engineType = src->get(i)->query<EngineTypeAnnotation>()->type();

				std::unique_ptr<raco::data_storage::ValueBase> uniqueValue;
				if (PropertyInterface::primitiveType(engineType) == PrimitiveType::Ref) {
					// References represent the various texture types which can't be linked
					uniqueValue = std::unique_ptr<raco::data_storage::ValueBase>(createDynamicProperty<>(engineType));
				} else {
					uniqueValue = std::unique_ptr<raco::data_storage::ValueBase>(createDynamicProperty<raco::core::LinkEndAnnotation>(engineType));
				}
				ValueBase* newValue = context.addProperty(destUniforms, name, std::move(uniqueValue));

				auto it = cachedUniformValues_.find(std::make_tuple(materialName, name, engineType));
				if (it != cachedUniformValues_.end()) {
					// use cached value
					ValueBase* cachedValue = it->second.get();
					if (PropertyInterface::primitiveType(engineType) == PrimitiveType::Ref) {
						// Special case for references: perform lookup in the project by object id
						// Needed because the object might have been deleted in the meantime and we don't
						// want to set a pointer to an invalid object here.
						SEditorObject cachedObject = nullptr;
						if (cachedValue->asRef()) {
							cachedObject = context.project()->getInstanceByID(cachedValue->asRef()->objectID());
						}
						*newValue = cachedObject;
					} else {
						*newValue = *cachedValue;
					}
				} else {
					// copy value from material
					*newValue = *src->get(i);
				}
			}
		}
	} else {
		context.removeAllProperties(destUniforms);
	}
}

void MeshNode::checkMeshMaterialAttributMatch(BaseContext& context) {
	SMesh mesh = std::dynamic_pointer_cast<Mesh>(*mesh_);
	SMaterial material = getMaterial(0);

	std::string errors;
	if (mesh && material && mesh->meshData()) {
		for (const auto& attrib : material->attributes()) {
			std::string name = attrib.name;

			static const std::unordered_map<raco::core::MeshData::VertexAttribDataType, EnginePrimitive> meshAttribTypeMap = {
				{raco::core::MeshData::VertexAttribDataType::VAT_Float, EnginePrimitive::Double},
				{raco::core::MeshData::VertexAttribDataType::VAT_Float2, EnginePrimitive::Vec2f},
				{raco::core::MeshData::VertexAttribDataType::VAT_Float3, EnginePrimitive::Vec3f},
				{raco::core::MeshData::VertexAttribDataType::VAT_Float4, EnginePrimitive::Vec4f}};

			int index = mesh->meshData()->attribIndex(name);
			if (index != -1) {
				auto meshAttribType = meshAttribTypeMap.at(mesh->meshData()->attribDataType(index));
				if (attrib.type != meshAttribType) {
					// types don't match
					errors += fmt::format("Attribute '{}' type mismatch: Material '{}' requires type '{}' but Mesh '{}' provides type '{}'.\n",
						name, material->objectName(), attrib.type, mesh->objectName(), meshAttribType);
				}
			} else {
				// attribute not found by name in mesh attributes
				errors += fmt::format("Attribute '{}' required by Material '{}' not found in Mesh '{}'.\n", name, material->objectName(), mesh->objectName());
			}
		}
		if (!errors.empty()) {
			errors = "Attribute mismatch:\n\n" + errors;
		}
	}

	context.errors().removeError(ValueHandle{shared_from_this()});
	if (!errors.empty()) {
		context.errors().addError(ErrorCategory::GENERAL, ErrorLevel::ERROR, ValueHandle{shared_from_this()}, errors);
	}
}

void MeshNode::onAfterContextActivated(BaseContext& context) {
	// This handlers is needed to cover a corner case during paste.
	// Normally BaseContext::performExternalFileReload will be called during paste and will in turn 
	// call onAfterReferencedObjectChanged. In these cases the additional onAfterContextActivated handler
	// will cause duplicate work.
	// In case the mesh and material references are lost during paste however the onAfterReferencedObjectChanged handler
	// will not be called and we need this onAfterContextActivated handler to update the dynamic properties.
	auto matnames = getMaterialNames();
	updateMaterialSlots(context, matnames);

	ValueHandle materialContHandle = ValueHandle(shared_from_this(), {"materials"})[0];
	if (materialContHandle) {
		ValueHandle uniformsHandle = materialContHandle.get("uniforms");
		if (uniformsHandle) {
			auto materialHandle = materialContHandle.get("material");
			auto material = materialHandle.asTypedRef<Material>();
			Table* materialUniforms = nullptr;
			if (material) {
				materialUniforms = &*material->uniforms_;
			}
			updateUniformContainer(context, materials_->get(0)->asTable().name(0), materialUniforms, uniformsHandle);
		}
	}
	
	checkMeshMaterialAttributMatch(context);
}

void MeshNode::onAfterReferencedObjectChanged(BaseContext& context, ValueHandle const& changedObject) {
	SMesh mesh = std::dynamic_pointer_cast<Mesh>(changedObject.rootObject());
	if (mesh) {
		std::vector<std::string> matnames = mesh->materialNames();
		updateMaterialSlots(context, matnames);
		checkMeshMaterialAttributMatch(context);
	}

	SMaterial material = std::dynamic_pointer_cast<Material>(changedObject.rootObject());
	if (material) {
		// TODO Multimaterial case: find all material slots using the material and update the uniforms:
		// Currently: single material: use the uniforms for the first material slot
		ValueHandle uniformsHandle = ValueHandle(shared_from_this(), {"materials"})[0].get("uniforms");
		updateUniformContainer(context, materials_->get(0)->asTable().name(0), &*material->uniforms_, uniformsHandle);
		checkMeshMaterialAttributMatch(context);
		context.changeMultiplexer().recordValueChanged(uniformsHandle);
	}
}

void MeshNode::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	ValueHandle meshHandle(shared_from_this(), {"mesh"});
	if (value == meshHandle) {
		auto matnames = getMaterialNames();
		updateMaterialSlots(context, matnames);
		checkMeshMaterialAttributMatch(context);
	}
	ValueHandle materialsHandle(shared_from_this(), {"materials"});
	if (materialsHandle.contains(value) && value.depth() == 3 && value.getPropName() == "material") {
		std::string materialName = value.parent().getPropName();
		ValueHandle uniformsHandle = value.parent().get("uniforms");
		const Table& uniforms = uniformsHandle.constValueRef()->asTable();

		SMaterial material = value.asTypedRef<Material>();
		Table* materialUniforms = nullptr;
		if (material) {
			materialUniforms = &*material->uniforms_;
		}
		updateUniformContainer(context, materialName, materialUniforms, uniformsHandle);
		checkMeshMaterialAttributMatch(context);
		context.changeMultiplexer().recordValueChanged(uniformsHandle);
	}
}

}  // namespace raco::user_types