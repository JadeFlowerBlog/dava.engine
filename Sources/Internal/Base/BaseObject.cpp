/*==================================================================================
    Copyright (c) 2008, DAVA Consulting, LLC
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the DAVA Consulting, LLC nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE DAVA CONSULTING, LLC AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL DAVA CONSULTING, LLC BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    Revision History:
        * Created by Vitaliy Borodovsky 
=====================================================================================*/
#include "Base/BaseObject.h"
#include "Base/ObjectFactory.h"
#include "FileSystem/KeyedArchive.h"
#include "Base/Introspection.h"

namespace DAVA
{
REGISTER_CLASS(BaseObject);    
    

const String & BaseObject::GetClassName()
{
    return ObjectFactory::Instance()->GetName(this);
}
    
/**
    \brief virtual function to save node to KeyedArchive
 */
void BaseObject::Save(KeyedArchive * archive)
{
    archive->SetString("##name", GetClassName());
    
    const IntrospectionInfo *info = GetIntrospection(this);
    if(info)
    {
        SaveIntrospection(GetClassName(), archive, info, this);
    }
}
    
void BaseObject::SaveIntrospection(const String &key, KeyedArchive * archive, const IntrospectionInfo *info, void *object)
{
    String keyPrefix = key;
    while(NULL != info)
    {
        keyPrefix += String(info->Name());
        for(int32 i = 0; i < info->MembersCount(); ++i)
        {
            const IntrospectionMember *member = info->Member(i);
			if(member)
			{
				const DAVA::MetaInfo *memberMetaInfo = member->Type();
                if(memberMetaInfo && memberMetaInfo->GetIntrospection())
                {
                    SaveIntrospection(keyPrefix + String(member->Name()), archive, memberMetaInfo->GetIntrospection(), member->Pointer(object));
                }
                else if(member->Flags() & INTROSPECTION_FLAG_SERIALIZABLE)
                {
                    VariantType value = member->Value(object);
                    archive->SetVariant(keyPrefix + String(member->Name()), &value);
                }
			}
        }
        
        info = info->BaseInfo();
    }
}
    

    
BaseObject * BaseObject::LoadFromArchive(KeyedArchive * archive)
{
    String name = archive->GetString("##name");
    BaseObject * object = ObjectFactory::Instance()->New(name);
    if (object)
    {
        object->Load(archive);
    }
    return object;
}

/**
    \brief virtual function to load node to KeyedArchive
 */
void BaseObject::Load(KeyedArchive * archive)
{
    const IntrospectionInfo *info = GetIntrospection(this);
    if(info)
    {
        LoadIntrospection(GetClassName(), archive, info, this);
    }
}
 
void BaseObject::LoadIntrospection(const String &key, KeyedArchive * archive, const IntrospectionInfo *info, void *object)
{
    String keyPrefix = key;
    while(NULL != info)
    {
        keyPrefix += String(info->Name());
        for(int32 i = 0; i < info->MembersCount(); ++i)
        {
            const IntrospectionMember *member = info->Member(i);
			if(member)
			{
				const DAVA::MetaInfo *memberMetaInfo = member->Type();
                if(memberMetaInfo && memberMetaInfo->GetIntrospection())
                {
                    LoadIntrospection(keyPrefix + String(member->Name()), archive, memberMetaInfo->GetIntrospection(), member->Pointer(object));
                }
                else if(member->Flags() & INTROSPECTION_FLAG_SERIALIZABLE)
                {
                    const VariantType * value = archive->GetVariant(keyPrefix + String(member->Name()));
                    if(value)
                    {
                        member->SetValue(object, *value);
                    }
                }
			}
        }
        
        info = info->BaseInfo();
    }
}
    
};


