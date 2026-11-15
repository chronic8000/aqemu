#ifndef STORE_TRIAL_H
#define STORE_TRIAL_H

#include <QString>

class QWidget;

/** Microsoft Store product page / in-app purchase. Windows Store builds only. */
void AQ_Store_Buy( QWidget *parent = nullptr );

/** "Trial: N days left", "Trial ends today", or empty when the licence is unknown or full. */
QString AQ_Store_Status_Text();

/** True when a Store licence was read and it is no longer active. */
bool AQ_Store_Licence_Inactive();

/** True when a Store licence was read and the app is fully purchased (not trial). */
bool AQ_Store_Is_Fully_Licensed();

#endif
