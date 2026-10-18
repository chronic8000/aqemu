#ifndef STORE_TRIAL_H
#define STORE_TRIAL_H

#include <QString>

/** Microsoft Store product page. Windows Store builds only. */
void AQ_Store_Buy();

/** "Trial: N days left", "Trial ends today", or empty when the licence is unknown or full. */
QString AQ_Store_Status_Text();

/** True when a Store licence was read and it is no longer active. */
bool AQ_Store_Licence_Inactive();

#endif
