/*
 Copyright (C) 2010-2017 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TrenchBroom_SetBrushFaceAttributesTool
#define TrenchBroom_SetBrushFaceAttributesTool

#include "View/Tool.h"
#include "View/ToolController.h"

#include <memory>

namespace TrenchBroom {
    namespace View {
        class MapDocument;

        class SetBrushFaceAttributesTool : public ToolControllerBase<NoPickingPolicy, NoKeyPolicy, MousePolicy, NoMouseDragPolicy, NoRenderPolicy, NoDropPolicy>, public Tool {
        private:
            std::weak_ptr<MapDocument> m_document;
        public:
            SetBrushFaceAttributesTool(std::weak_ptr<MapDocument> document);
        private:
            Tool* doGetTool() override;
            const Tool* doGetTool() const override;

            bool doMouseClick(const InputState& inputState) override;
            bool doMouseDoubleClick(const InputState& inputState) override;

            void copyAttributesFromSelection(const InputState& inputState, bool applyToBrush);
            bool canCopyAttributesFromSelection(const InputState& inputState) const;
            bool applies(const InputState& inputState) const;
            bool copyAllAttributes(const InputState& inputState) const;

            bool doCancel() override;
        };
    }
}

#endif /* defined(TrenchBroom_SetBrushFaceAttributesTool) */
