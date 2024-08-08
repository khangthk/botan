/*
* Runtime CPU detection for POWER/PowerPC
* (C) 2009,2010,2013,2017,2021 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/internal/cpuid.h>

#include <botan/internal/os_utils.h>

#if defined(BOTAN_TARGET_ARCH_IS_PPC64)

namespace Botan {

uint32_t CPUID::CPUID_Data::detect_cpu_features(uint32_t allowed) {
   uint32_t feat = 0;

   #if(defined(BOTAN_TARGET_OS_HAS_GETAUXVAL) || defined(BOTAN_TARGET_HAS_ELF_AUX_INFO))

   enum class PPC_hwcap_bit : uint64_t {
      ALTIVEC_bit = (1 << 28),
      CRYPTO_bit = (1 << 25),
      DARN_bit = (1 << 21),
   };

   const uint64_t hwcap_altivec = OS::get_auxval(16); // AT_HWCAP

   feat |= if_set(hwcap_altivec, PPC_hwcap_bit::ALTIVEC_bit, CPUID::CPUID_ALTIVEC_BIT, allowed);

   if(feat & CPUD::CPUID_ALTIVEC_BIT) {
      const uint64_t hwcap_crypto = OS::get_auxval(26); // AT_HWCAP2
      feat |= if_set(hwcap_crypto, PPC_hwcap_bit::CRYPTO_bit, CPUID::CPUID_POWER_CRYPTO_BIT, allowed);
      feat |= if_set(hwcap_crypto, PPC_hwcap_bit::DARN_bit, CPUID::CPUID_POWER_DARN_BIT, allowed);
   }

   #else

   auto vmx_probe = []() noexcept -> int {
      asm("vor 0, 0, 0");
      return 1;
   };

   auto vcipher_probe = []() noexcept -> int {
      asm("vcipher 0, 0, 0");
      return 1;
   };

   auto darn_probe = []() noexcept -> int {
      uint64_t output = 0;
      asm volatile("darn %0, 1" : "=r"(output));
      return (~output) != 0;
   };

   if(allowed & CPUID::CPUID_ALTIVEC_BIT) {
      if(OS::run_cpu_instruction_probe(vmx_probe) == 1) {
         feat |= CPUID::CPUID_ALTIVEC_BIT;
      }

      if(feat & CPUID::CPUID_ALTIVEC_BIT) {
         if(OS::run_cpu_instruction_probe(vcipher_probe) == 1) {
            feat |= CPUID::CPUID_POWER_CRYPTO_BIT & allowed;
         }

         if(OS::run_cpu_instruction_probe(darn_probe) == 1) {
            feat |= CPUID::CPUID_DARN_BIT & allowed;
         }
      }
   }

   #endif

   return feat;
}

}  // namespace Botan

#endif
