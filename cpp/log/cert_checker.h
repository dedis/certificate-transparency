/* -*- mode: c++; indent-tabs-mode: nil -*- */
#ifndef CERT_CHECKER_H
#define CERT_CHECKER_H

#include <openssl/cms.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "log/cert.h"
#include "util/status.h"
#include "util/statusor.h"

namespace cert_trans {

class Cert;
class CertChain;
class PreCertChain;

// A class for doing sanity-checks on log submissions before accepting them.
// We don't necessarily want to do full certificate verification
// before accepting them. E.g., we may want to accept submissions of
// invalid (say, expired) certificates directly from clients,
// to detect attacks after the fact. We primarily only
// want to check that submissions chain to a whitelisted CA, so that
// (1) we know where a cert is coming from; and
// (2) we get some spam protection.
class CertChecker {
 public:
  CertChecker() = default;

  virtual ~CertChecker();

  // Load a file of concatenated PEM-certs.
  // Returns true if at least one certificate was successfully loaded, and no
  // errors were encountered. Returns false otherwise (and will not load any
  // certificates from this file).
  virtual bool LoadTrustedCertificates(const std::string& trusted_cert_file);

  // Load directly from |trusted_certs|, a vector of PEM-certs.
  // Returns true if at least one of the supplied certs was loaded successfully.
  virtual bool LoadTrustedCertificates(
      const std::vector<std::string>& trusted_certs);

  virtual void ClearAllTrustedCertificates();

  virtual const std::multimap<std::string, const Cert*>&
  GetTrustedCertificates() const {
    return trusted_;
  }

  virtual size_t NumTrustedCertificates() const { return trusted_.size(); }

  // NOTE: CMS related API is provisional and may evolve over the near
  // future.
  // Checks that a CMS_ContentInfo has a signer that matches a specified
  // certificate. Does not verify the signature or check the payload.
  virtual util::StatusOr<bool> IsCmsSignedByCert(CMS_ContentInfo* cms,
                                                 const Cert& cert) const;

  // Unpacks a CMS signed data object that is assumed to contain a certificate
  // If the CMS signature verifies as being signed by the supplied Cert
  // then we return a corresponding new Cert object built from the unpacked
  // data. If it cannot be loaded as a certificate or fails CMS signing check
  // then an unloaded empty Cert object is returned.
  // The caller owns the returned certificate and must free the input bio.
  // NOTE: Certificate validity checks must be done separately. This
  // only checks that the CMS signature is validly made by the supplied
  // certificate.
  virtual Cert* UnpackCmsSignedCertificate(BIO* cms_bio_in,
                                           const Cert& verify_cert);

  // Check that:
  // (1) Each certificate is correctly signed by the next one in the chain; and
  // (2) The last certificate is issued by a certificate in our trusted store.
  // We do not check that the certificates are otherwise valid. In particular,
  // we accept certificates that have expired, are not yet valid, or have
  // critical extensions we do not recognize.
  // If verification succeeds, add the last self-signed cert to the chain
  // (or replace with store version) - the resulting chain is guaranteed to
  // contain at least one certificate. (Having exactly one certificate implies
  // someone is trying to log a root cert, which is fine though unexciting.)
  virtual util::Status CheckCertChain(CertChain* chain) const;

  // Check that:
  // (1) The PreCertChain is well-formed according to I-D rules.
  // (2) Each certificate is correctly signed by the next one in the chain; and
  // (3) The last certificate is issued by a certificate in our trusted store.
  // If verification succeeds, add the last self-signed cert to the chain
  // (or replace with store version) - the resulting chain is guaranteed to
  // contain at least two certificates (three if there is a Precert Signing
  // Certificate);
  // If valid, also fills in the |issuer_key_hash| and |tbs_certificate|.
  virtual util::Status CheckPreCertChain(PreCertChain* chain,
                                         std::string* issuer_key_hash,
                                         std::string* tbs_certificate) const;

 private:
  util::Status CheckIssuerChain(CertChain* chain) const;

  // Look issuer up from the trusted store, and verify signature.
  util::Status GetTrustedCa(CertChain* chain) const;

  // Returns true if the cert is trusted, false if it's not,
  // INVALID_ARGUMENT if something is wrong with the cert, and
  // INTERNAL if something terrible happened.
  util::StatusOr<bool> IsTrusted(const Cert& cert,
                                 std::string* subject_name) const;

  // Verifies that data from a DER BIO is signed by a given certificate.
  // and writes the unwrapped content to another BIO. NULL can be passed for
  // cms_bio_out if the caller just wishes to verify the signature. Does
  // not free either BIO. Does not do any checks on the content of the
  // CMS message or validate that the CMS signature is trusted to root.
  util::Status UnpackCmsDerBio(BIO* cms_bio_in, const Cert& certChain,
                               BIO* cms_bio_out);

  // A map by the DER encoding of the subject name.
  // All code manipulating this container must ensure contained elements are
  // deallocated appropriately.
  std::multimap<std::string, const Cert*> trusted_;

  // Helper for LoadTrustedCertificates, whether reading from file or memory.
  // Takes ownership of bio_in and frees it.
  bool LoadTrustedCertificatesFromBIO(BIO* bio_in);

  DISALLOW_COPY_AND_ASSIGN(CertChecker);
};

}  // namespace cert_trans

#endif
