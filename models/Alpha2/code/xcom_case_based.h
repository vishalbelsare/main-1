/**
 * @file   xcom_case_based.h
 * Declarations common to cross-compatible case-based models
 *  
 * Included in custom.h.
 */

#pragma once

#if !defined(OPENM)
namespace mm {
#endif

/**
 * Set or retrieve case info.
 * 
 * If called with arguments, records the case information. If called with no arguments, returns
 * the case information.  Defined in a framework module.
 *
 * @param [in,out] ci The case information for the simulation member.
 * @param reset       To indicate that there is no case information.
 *
 * @return null The case information for the simulation member.
 */
case_info* CaseInfo(case_info* ci = nullptr, bool reset = false);

#if !defined(OPENM)
}
#endif
