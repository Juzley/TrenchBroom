/*
 Copyright (C) 2010-2014 Kristian Duske
 
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

#ifndef __TrenchBroom__EntityProperties__
#define __TrenchBroom__EntityProperties__

#include "StringUtils.h"
#include "Model/ModelTypes.h"

#include <map>
#include <vector>

namespace TrenchBroom {
    namespace Assets {
        class EntityDefinition;
        class AttributeDefinition;
    }
    
    namespace Model {
        namespace AttributeNames {
            extern const AttributeName Classname;
            extern const AttributeName Origin;
            extern const AttributeName Wad;
            extern const AttributeName Wal;
            extern const AttributeName Mods;
            extern const AttributeName Spawnflags;
            extern const AttributeName EntityDefinitions;
            extern const AttributeName Angle;
            extern const AttributeName Angles;
            extern const AttributeName Mangle;
            extern const AttributeName Target;
            extern const AttributeName Targetname;
            extern const AttributeName Killtarget;
            extern const AttributeName GroupType;
            extern const AttributeName LayerName;
            extern const AttributeName Layer;
            extern const AttributeName GroupName;
            extern const AttributeName Group;
        }
        
        namespace AttributeValues {
            extern const AttributeValue WorldspawnClassname;
            extern const AttributeValue NoClassname;
            extern const AttributeValue LayerClassname;
            extern const AttributeValue GroupClassname;
            extern const AttributeValue GroupTypeLayer;
            extern const AttributeValue GroupTypeGroup;
        }

        String numberedAttributePrefix(const String& name);
        bool isNumberedAttribute(const String& prefix, const AttributeName& name);
        
        class EntityAttribute {
        public:
            typedef std::map<AttributableNode*, EntityAttribute> Map;
            typedef std::vector<EntityAttribute> List;
            static const List EmptyList;
        private:
            AttributeName m_name;
            AttributeValue m_value;
            const Assets::AttributeDefinition* m_definition;
        public:
            EntityAttribute();
            EntityAttribute(const AttributeName& name, const AttributeValue& value, const Assets::AttributeDefinition* definition = NULL);
            bool operator<(const EntityAttribute& rhs) const;
            int compare(const EntityAttribute& rhs) const;
            
            const AttributeName& name() const;
            const AttributeValue& value() const;
            const Assets::AttributeDefinition* definition() const;
            
            void setName(const AttributeName& name, const Assets::AttributeDefinition* definition);
            void setValue(const AttributeValue& value);
        };

        bool isLayer(const String& classname, const EntityAttribute::List& attributes);
        bool isGroup(const String& classname, const EntityAttribute::List& attributes);
        bool isWorldspawn(const String& classname, const EntityAttribute::List& attributes);
        const AttributeValue& findAttribute(const EntityAttribute::List& attributes, const AttributeName& name, const AttributeValue& defaultValue = EmptyString);
        
        class EntityAttributes {
        private:
            EntityAttribute::List m_attributes;
        public:
            const EntityAttribute::List& attributes() const;
            void setAttributes(const EntityAttribute::List& attributes);

            const EntityAttribute& addOrUpdateAttribute(const AttributeName& name, const AttributeValue& value, const Assets::AttributeDefinition* definition);
            void renameAttribute(const AttributeName& name, const AttributeName& newName, const Assets::AttributeDefinition* newDefinition);
            void removeAttribute(const AttributeName& name);
            void updateDefinitions(const Assets::EntityDefinition* entityDefinition);
            
            bool hasAttribute(const AttributeName& name) const;
            const AttributeValue* attribute(const AttributeName& name) const;
            const AttributeValue& safeAttribute(const AttributeName& name, const AttributeValue& defaultValue) const;
            
            EntityAttribute::List numberedAttributes(const String& prefix) const;
        private:
            EntityAttribute::List::const_iterator findAttribute(const AttributeName& name) const;
            EntityAttribute::List::iterator findAttribute(const AttributeName& name);
        };
    }
}

#endif /* defined(__TrenchBroom__EntityProperties__) */
