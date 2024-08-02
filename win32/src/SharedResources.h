/**
 * SharedResources.h
 * Resource definitions that should be shared among projects.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _RODENT_SHARED_RESOURCES_H
#define _RODENT_SHARED_RESOURCES_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Menus
#define IDM_BASE                        1000
#define IDM_BACK                        (IDM_BASE + 1)
#define IDM_NEXT                        (IDM_BASE + 2)
#define IDM_PARENT                      (IDM_BASE + 3)
#define IDM_REFRESH                     (IDM_BASE + 4)
#define IDM_STOP                        (IDM_BASE + 5)
#define IDM_GO                          (IDM_BASE + 6)

// Controls (Main Window)
#define IDC_BASE                        1100
#define IDC_CMBADDRESS                  (IDC_BASE + 1)
#define IDC_LSTDIRECTORY                (IDC_BASE + 2)
#define IDC_TBMAIN                      (IDC_BASE + 3)
#define IDC_TBADDRESS                   (IDC_BASE + 4)
#define IDC_RBMAIN                      (IDC_BASE + 5)
#define IDC_STATUSBAR                   (IDC_BASE + 6)

// Controls (Downloading Dialog)
#define IDC_BTOPENFOLDER                (IDC_BASE + 7)
#define IDC_LBLURL                      (IDC_BASE + 8)
#define IDC_LBLPATH                     (IDC_BASE + 9)
#define IDC_LBLSIZE                     (IDC_BASE + 10)

// Timers
#define IDT_BASE                        1200
#define IDT_LOADING                     (IDT_BASE + 1)

#endif // _RODENT_SHARED_RESOURCES_H
