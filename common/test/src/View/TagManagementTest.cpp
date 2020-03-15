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

#include <gtest/gtest.h>

#include "Logger.h"
#include "IO/Path.h"
#include "IO/TestEnvironment.h"
#include "Assets/Texture.h"
#include "Assets/TextureCollection.h"
#include "Assets/TextureManager.h"
#include "Model/BrushNode.h"
#include "Model/BrushFace.h"
#include "Model/ChangeBrushFaceAttributesRequest.h"
#include "Model/EntityNode.h"
#include "Model/LayerNode.h"
#include "Model/Tag.h"
#include "Model/TagMatcher.h"
#include "Model/TestGame.h"
#include "View/MapDocumentTest.h"

#include <vector>

namespace TrenchBroom {
    namespace View {
        class TagManagementTest : public MapDocumentTest {
        protected:
            Assets::Texture* m_matchingTexture;
            Assets::Texture* m_nonMatchingTexture;
            Assets::TextureCollection* m_textureCollection;
        protected:
            void SetUp() override {
                MapDocumentTest::SetUp();

                auto matchingTexture = std::make_unique<Assets::Texture>("some_texture", 16, 16);
                auto nonMatchingTexture = std::make_unique<Assets::Texture>("other_texture", 32, 32);

                matchingTexture->setSurfaceParms({"some_parm"});

                auto textureCollection = std::make_unique<Assets::TextureCollection>(std::vector<Assets::Texture*>({
                    matchingTexture.get(),
                    nonMatchingTexture.get()
                }));

                document->textureManager().setTextureCollections(std::vector<Assets::TextureCollection*>({
                    textureCollection.get()
                }));

                m_matchingTexture = matchingTexture.release();
                m_nonMatchingTexture = nonMatchingTexture.release();
                m_textureCollection = textureCollection.release();

                game->setSmartTags({
                    Model::SmartTag("texture", {}, std::make_unique<Model::TextureNameTagMatcher>("some_texture")),
                    Model::SmartTag("surfaceparm", {}, std::make_unique<Model::SurfaceParmTagMatcher>("some_parm")),
                    Model::SmartTag("contentflags", {}, std::make_unique<Model::ContentFlagsTagMatcher>(1)),
                    Model::SmartTag("surfaceflags", {}, std::make_unique<Model::SurfaceFlagsTagMatcher>(1)),
                    Model::SmartTag("entity", {}, std::make_unique<Model::EntityClassNameTagMatcher>("brush_entity", ""))
                });
                document->registerSmartTags();
            }
        };

        class TestCallback : public Model::TagMatcherCallback {
        private:
            size_t m_option;
        public:
            explicit TestCallback(const size_t option) :
            m_option(option) {}

            size_t selectOption(const std::vector<std::string>& /* options */) {
                return m_option;
            }
        };

        TEST_F(TagManagementTest, tagRegistration) {
            ASSERT_TRUE(document->isRegisteredSmartTag("texture"));
            ASSERT_TRUE(document->isRegisteredSmartTag("surfaceparm"));
            ASSERT_TRUE(document->isRegisteredSmartTag("contentflags"));
            ASSERT_TRUE(document->isRegisteredSmartTag("surfaceflags"));
            ASSERT_TRUE(document->isRegisteredSmartTag("entity"));
            ASSERT_FALSE(document->isRegisteredSmartTag(""));
            ASSERT_FALSE(document->isRegisteredSmartTag("asdf"));
        }

        // https://github.com/kduske/TrenchBroom/issues/2905
        TEST_F(TagManagementTest, duplicateTag) {
            game->setSmartTags({
                Model::SmartTag("texture", {}, std::make_unique<Model::TextureNameTagMatcher>("some_texture")),
                Model::SmartTag("texture", {}, std::make_unique<Model::SurfaceParmTagMatcher>("some_other_texture")),
            });
            ASSERT_THROW(document->registerSmartTags(), std::logic_error);
        }

        TEST_F(TagManagementTest, matchTextureNameTag) {
            auto matchingBrushNode = std::unique_ptr<Model::BrushNode>(createBrushNode("some_texture"));
            auto nonMatchingBrushNode = std::unique_ptr<Model::BrushNode>(createBrushNode("asdf"));

            const auto& tag = document->smartTag("texture");
            for (const auto* face : matchingBrushNode->brush().faces()) {
                ASSERT_TRUE(tag.matches(*face));
            }
            for (const auto* face : nonMatchingBrushNode->brush().faces()) {
                ASSERT_FALSE(tag.matches(*face));
            }
        }

        TEST_F(TagManagementTest, enableTextureNameTag) {
            auto* nonMatchingBrushNode = createBrushNode("asdf");
            document->addNode(nonMatchingBrushNode, document->currentParent());

            const auto& tag = document->smartTag("texture");
            ASSERT_TRUE(tag.canEnable());

            auto* face = nonMatchingBrushNode->brush().faces().front();
            ASSERT_FALSE(tag.matches(*face));

            document->select({ nonMatchingBrushNode, face });

            TestCallback callback(0);
            tag.enable(callback, *document);

            ASSERT_TRUE(tag.matches(*face));
        }

        TEST_F(TagManagementTest, disableTextureNameTag) {
            const auto& tag = document->smartTag("texture");
            ASSERT_FALSE(tag.canDisable());
        }

        TEST_F(TagManagementTest, matchSurfaceParmTag) {
            auto texture = std::make_unique<Assets::Texture>("texturename", 16, 16);
            texture->setSurfaceParms({"some_parm"});

            auto matchingBrushNode = std::unique_ptr<Model::BrushNode>(createBrushNode("some_texture"));
            auto nonMatchingBrushNode = std::unique_ptr<Model::BrushNode>(createBrushNode("asdf"));

            for (auto* face : matchingBrushNode->brush().faces()) {
                face->setTexture(texture.get());
            }

            const auto& tag = document->smartTag("surfaceparm");
            for (const auto* face : matchingBrushNode->brush().faces()) {
                ASSERT_TRUE(tag.matches(*face));
            }
            for (const auto* face : nonMatchingBrushNode->brush().faces()) {
                ASSERT_FALSE(tag.matches(*face));
            }
        }

        TEST_F(TagManagementTest, enableSurfaceParmTag) {
            const auto& tag = document->smartTag("surfaceparm");
            ASSERT_FALSE(tag.canEnable());
        }

        TEST_F(TagManagementTest, disableSurfaceParmTag) {
            const auto& tag = document->smartTag("surfaceparm");
            ASSERT_FALSE(tag.canDisable());
        }

        TEST_F(TagManagementTest, matchContentFlagsTag) {
            auto matchingBrushNode = std::unique_ptr<Model::BrushNode>(createBrushNode("asdf"));
            auto nonMatchingBrushNode = std::unique_ptr<Model::BrushNode>(createBrushNode("asdf"));

            for (auto* face : matchingBrushNode->brush().faces()) {
                face->setSurfaceContents(1);
            }
            for (auto* face : nonMatchingBrushNode->brush().faces()) {
                face->setSurfaceContents(2);
            }

            const auto& tag = document->smartTag("contentflags");
            for (const auto* face : matchingBrushNode->brush().faces()) {
                ASSERT_TRUE(tag.matches(*face));
            }
            for (const auto* face : nonMatchingBrushNode->brush().faces()) {
                ASSERT_FALSE(tag.matches(*face));
            }
        }

        TEST_F(TagManagementTest, enableContentFlagsTag) {
            auto* nonMatchingBrushNode = createBrushNode("asdf");
            document->addNode(nonMatchingBrushNode, document->currentParent());

            const auto& tag = document->smartTag("contentflags");
            ASSERT_TRUE(tag.canEnable());

            auto* face = nonMatchingBrushNode->brush().faces().front();
            ASSERT_FALSE(tag.matches(*face));

            document->select({ nonMatchingBrushNode, face });

            TestCallback callback(0);
            tag.enable(callback, *document);

            ASSERT_TRUE(tag.matches(*face));
        }

        TEST_F(TagManagementTest, disableContentFlagsTag) {
            auto* matchingBrushNode = createBrushNode("asdf");
            for (auto* face : matchingBrushNode->brush().faces()) {
                face->setSurfaceContents(1);
            }

            document->addNode(matchingBrushNode, document->currentParent());

            const auto& tag = document->smartTag("contentflags");
            ASSERT_TRUE(tag.canDisable());

            auto* face = matchingBrushNode->brush().faces().front();
            ASSERT_TRUE(tag.matches(*face));

            document->select({ matchingBrushNode, face });

            TestCallback callback(0);
            tag.disable(callback, *document);

            ASSERT_FALSE(tag.matches(*face));
        }

        TEST_F(TagManagementTest, matchSurfaceFlagsTag) {
            auto matchingBrushNode = std::unique_ptr<Model::BrushNode>(createBrushNode("asdf"));
            auto nonMatchingBrushNode = std::unique_ptr<Model::BrushNode>(createBrushNode("asdf"));

            for (auto* face : matchingBrushNode->brush().faces()) {
                face->setSurfaceFlags(1);
            }
            for (auto* face : nonMatchingBrushNode->brush().faces()) {
                face->setSurfaceFlags(2);
            }

            const auto& tag = document->smartTag("surfaceflags");
            for (const auto* face : matchingBrushNode->brush().faces()) {
                ASSERT_TRUE(tag.matches(*face));
            }
            for (const auto* face : nonMatchingBrushNode->brush().faces()) {
                ASSERT_FALSE(tag.matches(*face));
            }
        }
        TEST_F(TagManagementTest, enableSurfaceFlagsTag) {
            auto* nonMatchingBrushNode = createBrushNode("asdf");
            document->addNode(nonMatchingBrushNode, document->currentParent());

            const auto& tag = document->smartTag("surfaceflags");
            ASSERT_TRUE(tag.canEnable());

            auto* face = nonMatchingBrushNode->brush().faces().front();
            ASSERT_FALSE(tag.matches(*face));

            document->select({ nonMatchingBrushNode, face });

            TestCallback callback(0);
            tag.enable(callback, *document);

            ASSERT_TRUE(tag.matches(*face));
        }

        TEST_F(TagManagementTest, disableSurfaceFlagsTag) {
            auto* matchingBrushNode = createBrushNode("asdf");
            for (auto* face : matchingBrushNode->brush().faces()) {
                face->setSurfaceFlags(1);
            }

            document->addNode(matchingBrushNode, document->currentParent());

            const auto& tag = document->smartTag("surfaceflags");
            ASSERT_TRUE(tag.canDisable());

            auto* face = matchingBrushNode->brush().faces().front();
            ASSERT_TRUE(tag.matches(*face));

            document->select({ matchingBrushNode, face });

            TestCallback callback(0);
            tag.disable(callback, *document);

            ASSERT_FALSE(tag.matches(*face));
        }

        TEST_F(TagManagementTest, matchEntityClassnameTag) {
            auto* matchingBrushNode = createBrushNode("asdf");
            auto* nonMatchingBrushNode = createBrushNode("asdf");

            auto matchingEntity = std::make_unique<Model::EntityNode>();
            matchingEntity->addOrUpdateAttribute("classname", "brush_entity");
            matchingEntity->addChild(matchingBrushNode);

            auto nonMatchingEntity = std::make_unique<Model::EntityNode>();
            nonMatchingEntity->addOrUpdateAttribute("classname", "something");
            nonMatchingEntity->addChild(nonMatchingBrushNode);

            const auto& tag = document->smartTag("entity");
            ASSERT_TRUE(tag.matches(*matchingBrushNode));
            ASSERT_FALSE(tag.matches(*nonMatchingBrushNode));
        }

        TEST_F(TagManagementTest, enableEntityClassnameTag) {
            auto* brushNode = createBrushNode("asdf");
            document->addNode(brushNode, document->currentParent());

            const auto& tag = document->smartTag("entity");
            ASSERT_FALSE(tag.matches(*brushNode));

            ASSERT_TRUE(tag.canEnable());

            document->select(brushNode);

            TestCallback callback(0);
            tag.enable(callback, *document);
            ASSERT_TRUE(tag.matches(*brushNode));
        }

        TEST_F(TagManagementTest, enableEntityClassnameTagRetainsAttributes) {
            auto* brushNode = createBrushNode("asdf");

            auto* oldEntity = new Model::EntityNode();
            oldEntity->addOrUpdateAttribute("classname", "something");
            oldEntity->addOrUpdateAttribute("some_attr", "some_value");

            document->addNode(oldEntity, document->currentParent());
            document->addNode(brushNode, oldEntity);

            const auto& tag = document->smartTag("entity");
            document->select(brushNode);

            TestCallback callback(0);
            tag.enable(callback, *document);
            ASSERT_TRUE(tag.matches(*brushNode));

            auto* newEntity = brushNode->entity();
            ASSERT_NE(oldEntity, newEntity);

            ASSERT_NE(nullptr, newEntity);
            ASSERT_TRUE(newEntity->hasAttribute("some_attr"));
            ASSERT_EQ("some_value", newEntity->attribute("some_attr", ""));
        }

        TEST_F(TagManagementTest, disableEntityClassnameTag) {
            auto* brushNode = createBrushNode("asdf");

            auto* oldEntity = new Model::EntityNode();
            oldEntity->addOrUpdateAttribute("classname", "brush_entity");

            document->addNode(oldEntity, document->currentParent());
            document->addNode(brushNode, oldEntity);

            const auto& tag = document->smartTag("entity");
            ASSERT_TRUE(tag.matches(*brushNode));

            ASSERT_TRUE(tag.canDisable());

            document->select(brushNode);

            TestCallback callback(0);
            tag.disable(callback, *document);
            ASSERT_FALSE(tag.matches(*brushNode));
        }

        TEST_F(TagManagementTest, tagInitializeBrushTags) {
            auto* entityNode = new Model::EntityNode();
            entityNode->addOrUpdateAttribute("classname", "brush_entity");
            document->addNode(entityNode, document->currentParent());

            auto* brush = createBrushNode("some_texture");
            document->addNode(brush, entityNode);

            const auto& tag = document->smartTag("entity");
            ASSERT_TRUE(brush->hasTag(tag));
        }

        TEST_F(TagManagementTest, tagRemoveBrushTags) {
            auto* entityNode = new Model::EntityNode();
            entityNode->addOrUpdateAttribute("classname", "brush_entity");
            document->addNode(entityNode, document->currentParent());

            auto* brush = createBrushNode("some_texture");
            document->addNode(brush, entityNode);

            document->removeNode(brush);

            const auto& tag = document->smartTag("entity");
            ASSERT_FALSE(brush->hasTag(tag));
        }

        TEST_F(TagManagementTest, tagUpdateBrushTags) {
            auto* brushNode = createBrushNode("some_texture");
            document->addNode(brushNode, document->currentParent());

            auto* entity = new Model::EntityNode();
            entity->addOrUpdateAttribute("classname", "brush_entity");
            document->addNode(entity, document->currentParent());

            const auto& tag = document->smartTag("entity");
            ASSERT_FALSE(brushNode->hasTag(tag));

            document->reparentNodes(entity, { brushNode });
            ASSERT_TRUE(brushNode->hasTag(tag));
        }

        TEST_F(TagManagementTest, tagUpdateBrushTagsAfterReparenting) {
            auto* lightEntityNode = new Model::EntityNode();
            lightEntityNode->addOrUpdateAttribute("classname", "brush_entity");
            document->addNode(lightEntityNode, document->currentParent());

            auto* otherEntityNode = new Model::EntityNode();
            otherEntityNode->addOrUpdateAttribute("classname", "other");
            document->addNode(otherEntityNode, document->currentParent());

            auto* brushNode = createBrushNode("some_texture");
            document->addNode(brushNode, otherEntityNode);

            const auto& tag = document->smartTag("entity");
            ASSERT_FALSE(brushNode->hasTag(tag));

            document->reparentNodes(lightEntityNode, { brushNode });
            ASSERT_TRUE(brushNode->hasTag(tag));
        }

        TEST_F(TagManagementTest, tagUpdateBrushTagsAfterChangingClassname) {
            auto* lightEntityNode = new Model::EntityNode();
            lightEntityNode->addOrUpdateAttribute("classname", "asdf");
            document->addNode(lightEntityNode, document->currentParent());

            auto* brushNode = createBrushNode("some_texture");
            document->addNode(brushNode, lightEntityNode);

            const auto& tag = document->smartTag("entity");
            ASSERT_FALSE(brushNode->hasTag(tag));

            document->select(lightEntityNode);
            document->setAttribute("classname", "brush_entity");
            document->deselectAll();

            ASSERT_TRUE(brushNode->hasTag(tag));
        }

        TEST_F(TagManagementTest, tagInitializeBrushFaceTags) {
            auto* brushNodeWithTags = createBrushNode("some_texture");
            document->addNode(brushNodeWithTags, document->currentParent());

            const auto& tag = document->smartTag("texture");
            for (const auto* face : brushNodeWithTags->brush().faces()) {
                ASSERT_TRUE(face->hasTag(tag));
            }

            auto* brushNodeWithoutTags = createBrushNode("asdf");
            document->addNode(brushNodeWithoutTags, document->currentParent());

            for (const auto* face : brushNodeWithoutTags->brush().faces()) {
                ASSERT_FALSE(face->hasTag(tag));
            }
        }

        TEST_F(TagManagementTest, tagRemoveBrushFaceTags) {
            auto* brushNodeWithTags = createBrushNode("some_texture");
            document->addNode(brushNodeWithTags, document->currentParent());
            document->removeNode(brushNodeWithTags);

            const auto& tag = document->smartTag("texture");
            for (const auto* face : brushNodeWithTags->brush().faces()) {
                ASSERT_FALSE(face->hasTag(tag));
            }
        }

        TEST_F(TagManagementTest, tagUpdateBrushFaceTags) {
            auto* brushNode = createBrushNode("asdf");
            document->addNode(brushNode, document->currentParent());

            const auto& tag = document->smartTag("contentflags");

            auto* face = brushNode->brush().faces().front();
            ASSERT_FALSE(face->hasTag(tag));

            Model::ChangeBrushFaceAttributesRequest request;
            request.setContentFlag(0);

            document->select({ brushNode, face });
            document->setFaceAttributes(request);
            document->deselectAll();

            for (const auto* f : brushNode->brush().faces()) {
                if (f == face) {
                    ASSERT_TRUE(f->hasTag(tag));
                } else {
                    ASSERT_FALSE(f->hasTag(tag));
                }
            }
        }
    }
}
