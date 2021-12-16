/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ObjectTreeViewDefaultModel.h"

namespace raco::object_tree::model {

class ObjectTreeViewExternalProjectModel : public ObjectTreeViewDefaultModel {
	Q_OBJECT

public:
	class ProjectNode : public raco::core::EditorObject {
	public:
		static inline const TypeDescriptor typeDescription{"Project", false};
		TypeDescriptor const& getTypeDescription() const override {
			return typeDescription;
		}

		ProjectNode(ProjectNode const& other) : EditorObject(other) {
			fillPropertyDescription();
		}

		ProjectNode(const std::string& name, const std::string& id = std::string()) : EditorObject(name, id) {
			fillPropertyDescription();
		}

		ProjectNode() : ProjectNode("Main") {}

		void fillPropertyDescription() {
		}
	};

	ObjectTreeViewExternalProjectModel(raco::core::CommandInterface* commandInterface, core::FileChangeMonitor* fileChangeMonitor, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectsStoreInterface);

	QVariant data(const QModelIndex& index, int role) const override;

	void addProject(const QString& projectPath);
	void removeProjectsAtIndices(const QModelIndexList& indices);
	bool canRemoveProjectsAtIndices(const QModelIndexList& indices);

	Qt::TextElideMode textElideMode() const override;

	bool canCopyAtIndices(const QModelIndexList& indices) const override;
	bool canDeleteAtIndices(const QModelIndexList& indices) const override;

	bool isObjectAllowedIntoIndex(const QModelIndex& index, const core::SEditorObject& obj) const override;
	bool canPasteIntoIndex(const QModelIndex& index, const std::vector<core::SEditorObject>& objects, const std::set<std::string>& sourceProjectTopLevelObjectIds, bool asExtRef = false) const override;

public Q_SLOTS:
	size_t deleteObjectsAtIndices(const QModelIndexList& index) override;
	void copyObjectsAtIndices(const QModelIndexList& indices, bool deepCopy) override;
	void cutObjectsAtIndices(const QModelIndexList& indices, bool deepCut) override;
	bool pasteObjectAtIndex(const QModelIndex& index, bool pasteAsExtref = false, std::string* outError = nullptr, const std::string& serializedObjects = RaCoClipboard::get()) override;

protected:
	void buildObjectTree() override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;
	QMimeData* mimeData(const QModelIndexList& indexes) const override;


	std::string getOriginProjectPathOfSelectedIndex(const QModelIndex& index) const;

	components::Subscription projectChangedSubscription_;
};

}  // namespace raco::object_tree::model