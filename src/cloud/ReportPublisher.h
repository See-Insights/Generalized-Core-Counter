#pragma once
#include "Particle.h"
#include "time/HibernateWakeDiagnostics.h"
void UbidotsHandler(const char *event, const char *data);
void publishHibernateWakeForensics(const HibernateWakeDiagnostics::EventFields &fields);
