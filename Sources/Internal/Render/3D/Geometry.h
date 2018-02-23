#pragma once

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Render/3D/PolygonGroup.h"
#include "Asset/AssetBase.h"

namespace DAVA
{
class Geometry : public AssetBase
{
    ~Geometry();
    Geometry();

    void AddPolygonGroup(PolygonGroup* pgroup);
    PolygonGroup* GetPolygonGroup(uint32 index) const;

    void Load(const FilePath& filepath) override;
    void Save(const FilePath& filepath) override;
    void Reload() override;

protected:
    Vector<PolygonGroup*> geometries;
};
};
