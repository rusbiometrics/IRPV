/*
 * Image recognition performance verification
 *
 * This software is not subject to copyright protection
 */

#include <cstring>

#include "nullimplirpv11.h"

using namespace std;
using namespace IRPV;

NullImplIRPV11::NullImplIRPV11() {}

NullImplIRPV11::~NullImplIRPV11() {}

ReturnStatus
NullImplIRPV11::initialize(const std::string &configDir)
{
    return ReturnStatus(ReturnCode::Success);
}

ReturnStatus
NullImplIRPV11::createTemplate(const Image &image,
        TemplateRole role,
        std::vector<uint8_t> &templ)
{
    string blurb{"Long time ago in a galaxy far far away...\n"};

    templ.resize(blurb.size());
    memcpy(templ.data(), blurb.c_str(), blurb.size());

    return ReturnStatus(ReturnCode::Success);
}

ReturnStatus
NullImplIRPV11::matchTemplates(
        const std::vector<uint8_t> &verifTemplate,
        const std::vector<uint8_t> &enrollTemplate,
        double &similarity)
{
    similarity = std::rand();
    return ReturnStatus(ReturnCode::Success);
}

std::shared_ptr<VerifInterface>
VerifInterface::getImplementation()
{
    return std::make_shared<NullImplIRPV11>();
}





