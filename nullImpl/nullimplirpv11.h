/*
 * Face Recognition Performance Test
 *
 * This software is not subject to copyright protection and is in the public domain.
 */

#ifndef NULLIMPLIRPV11_H_
#define NULLIMPLIRPV11_H_

#include "irpv.h"

/*
 * Declare the implementation class of the IRPV VERIF (1:1) Interface
 */
namespace IRPV {
	
    class NullImplIRPV11 : public IRPV::VerifInterface {
public:

    NullImplIRPV11();
    ~NullImplIRPV11() override;

    ReturnStatus
    initialize(const std::string &configDir) override;

    ReturnStatus
    createTemplate(
            const Image &image,
            TemplateRole role,
            std::vector<uint8_t> &templ) override;

    ReturnStatus
    matchTemplates(
            const std::vector<uint8_t> &verifTemplate,
            const std::vector<uint8_t> &enrollTemplate,
            double &similarity) override;

private:
    // Some other members
};
}

#endif /* NULLIMPLIRPV11_H_ */
