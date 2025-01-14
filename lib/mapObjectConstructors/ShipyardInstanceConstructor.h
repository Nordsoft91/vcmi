/*
* ShipyardInstanceConstructor.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CDefaultObjectTypeHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGShipyard;

class ShipyardInstanceConstructor final : public CDefaultObjectTypeHandler<CGShipyard>
{
	JsonNode parameters;

protected:
	void initTypeData(const JsonNode & config) override;
	void initializeObject(CGShipyard * object) const override;

public:
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<AObjectTypeHandler&>(*this);
		h & parameters;
	}
};

VCMI_LIB_NAMESPACE_END
