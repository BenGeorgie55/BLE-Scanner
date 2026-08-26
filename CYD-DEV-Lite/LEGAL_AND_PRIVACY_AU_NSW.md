# Legal, Privacy and Responsible-Use Notice

**Australia / New South Wales focus**  
**Technical information only — not legal advice**

This document explains legal, privacy and responsible-use issues relevant to this firmware. It does not certify that every deployment is lawful. Legality depends on the facts, the jurisdiction, who operates the device, what information is collected, how it is used and retained, and the policies applying at the deployment site.

## Purpose and detection scope

This project is a BLE radio-awareness tool. It is designed primarily to identify or flag observable Bluetooth Low Energy advertising characteristics associated with **consumer-grade and commercially available devices**, including supported smart glasses, cameras, microphones, recording devices and related products.

It is **not represented as a reliable detector for military-grade, intelligence-grade, specialist government, highly specialised covert, or purpose-built professional surveillance equipment**.

Advanced equipment may:
- avoid BLE entirely;
- stop advertising;
- use proprietary or non-BLE radio systems;
- minimise radio emissions;
- rotate or obfuscate identifying fields;
- use changing or encrypted payloads;
- use wired, Wi-Fi, cellular or other communications.

**No alert does not mean no surveillance or recording equipment is present.**

## What an alert means

An alert means only that a received BLE advertisement matched a configured rule or reference condition.

An alert does **not** establish:
- who owns a device;
- who is carrying or wearing it;
- whether a camera is active;
- whether a microphone is active;
- whether recording is occurring;
- whether a device is hidden;
- exact physical distance;
- malicious intent;
- unlawful conduct.

Do not use an alert by itself to accuse, confront, search, detain, discipline or identify a person.

## BLE advertising and access to nearby devices

Normal operation analyses observable BLE advertising metadata. The detector does not need to pair with nearby devices or access their files, photos, messages, microphone, camera or stored content.

BLE advertising is a normal protocol mechanism used by devices that choose to advertise. Not every BLE device advertises continuously, and not every advertisement reveals enough information for useful classification.

## NSW Surveillance Devices Act

The NSW Surveillance Devices Act 2007 regulates listening devices, optical surveillance devices, tracking devices, data-surveillance devices and certain records/information obtained through unlawful surveillance.

This firmware is designed to analyse BLE advertising metadata rather than record private conversations, visually observe private activity, capture computer input/output, or determine a person's geographical location.

That design distinction is important, but **this document does not declare the firmware or every possible use exempt from the Act**. A user can change the legal character of a system through modifications, deployment practices, additional sensors, data combination or an unlawful purpose.

Do not modify or use the project to covertly record private conversations or activities, access computer data, or track people without understanding the law that applies.

## NSW workplace use

The Workplace Surveillance Act 2005 regulates surveillance of employees at work, including camera, computer and tracking surveillance, and contains notice/policy requirements and restrictions on covert surveillance.

A BLE environment-awareness deployment is not automatically employee surveillance. However, if an employer uses this project, its logs, or a combined system to monitor employees, vehicles, movements, work activity or computer use, workplace-surveillance obligations may become relevant.

Employers should obtain appropriate legal/HR/privacy review before operational deployment and should not use this open-source documentation as a substitute for required employee notices, policies or authorisations.

## Privacy and personal information

Under NSW privacy law, whether information is "personal information" can depend on whether an individual's identity is apparent or can reasonably be ascertained from the information.

The project therefore follows data-minimisation principles:
- raw observed BLE MAC addresses are not intentionally persisted;
- raw addresses may be used transiently in RAM where immediate processing requires them;
- retained device-correlation values are session-scoped/pseudonymous rather than permanent cross-session identities;
- session pseudonymisation is not the same as guaranteed anonymity;
- advertised names, manufacturer payloads and other radio fields can sometimes distinguish a device;
- exported logs should be treated as potentially sensitive operational data.

Avoid combining BLE observations with names, student/staff rosters, CCTV, access-control data, patient records, visitor registers or other identifying datasets unless there is a specific lawful and authorised reason.

## Schools, childcare and after-school care

Deployment should be authorised by the school, service provider or responsible organisation and follow its privacy, safeguarding, ICT, records-management and security policies.

For education-and-care services operating under the National Quality Framework, the Children (Education and Care Services) National Law (NSW) contains privacy provisions and applies Commonwealth privacy law in that framework.

Public schools and public-sector bodies may also have NSW public-sector privacy obligations. Other schools or organisations may have Commonwealth Privacy Act obligations depending on their circumstances.

This project should not be used to build profiles of students, children, parents, staff or visitors.

## Healthcare and aged care

Healthcare and aged-care settings can involve heightened privacy and confidentiality duties.

In NSW, the Health Records and Information Privacy Act 2002 applies to organisations that provide health services or collect, hold or use health information and requires compliance with Health Privacy Principles where applicable.

BLE observations should not be linked to patient, resident or health records unless there is a clear lawful basis, a defined purpose, appropriate safeguards and organisational approval.

## Prisons, correctional centres and secure facilities

Use in a prison, correctional centre, secure facility, critical environment or controlled-access location must be explicitly authorised by the facility operator and comply with site security, radio, privacy and operational rules.

Possessing open-source hardware does not itself create authority to introduce or operate it inside a controlled facility.

## Australian radio and equipment compliance

The ACMA's current Low Interference Potential Devices framework authorises operation of many short-range radio devices subject to the applicable licence and technical conditions. ACMA also imposes equipment, EMC, electromagnetic-energy and labelling requirements in relevant circumstances.

Users should operate compliant ESP32/module hardware and avoid assuming that a modified radio system remains compliant merely because the underlying development board was sold in Australia.

**External antennas, higher-gain antennas, RF modifications and changes to transmit power can affect EIRP and equipment-compliance assumptions.** The complete hardware configuration should be checked where radio hardware is materially changed.

## Data retention and exports

If the firmware retains observations:
- define a legitimate purpose for retention;
- retain only what is needed;
- restrict access;
- protect exported files;
- do not create a cross-session identity database;
- delete records when no longer required by the purpose or applicable records obligations.

## Accuracy and operational limitations

Configured confidence values are rule-match scores, not scientifically validated probabilities of product identity.

RSSI is affected by antennas, orientation, obstructions, reflections, transmit power and the RF environment. It must not be treated as proof of exact physical distance.

## Jurisdiction

This legal file focuses on Australia and New South Wales. Users in other jurisdictions must check their own law.


## Build-specific scope — CYD DEV Lite

CYD DEV Lite is the hardware-validated standalone facility/development build. Its legal/privacy profile is different from the portable units because it supports **SD logging, detailed review, manual context capture and Sentry Mode**.

### SD logging

The SD card may retain observations, classifications, counters, session records and session-scoped pseudonymous values. Raw observed BLE MAC addresses must not be intentionally persisted.

For organisational deployment:
- restrict access to the SD card;
- define retention/deletion rules;
- avoid unnecessary duplication of exports;
- secure backups and copied CSV/log files;
- do not merge records with identifying organisational datasets without a lawful, documented purpose.

### Sentry Mode

Sentry performs repeated environmental BLE measurements for baseline building, long-duration observation and database development.

A Sentry trigger means the BLE environment changed significantly. It does **not** prove that a smart-glasses, camera, microphone or recording device entered the area.

Because Sentry supports long-duration collection, operators should specifically assess workplace notice, organisational privacy, records-management and retention obligations before unattended deployment.

### Manual capture

Manual context capture is a technical research function. It should be used only where the operator has authority to collect the relevant radio-environment observations. It must not be used to create permanent person/device profiles.

### Private-address fail-closed design

CYD DEV Lite is designed to block active BLE scanning when private-address setup/verification fails. This is a privacy safeguard. It does not make the scanner invisible, untraceable or automatically lawful in every deployment.

Recommended public positioning: authorised school/childcare/health/aged-care/workplace/facility privacy-awareness, field research and database development where local policies permit.

# Authoritative Legal / Regulatory References

Checked for this documentation on **26 August 2026**.

These links are provided so users can verify the current law and regulator guidance. Legislation and guidance can change.

## New South Wales

- Surveillance Devices Act 2007 No 64  
  https://legislation.nsw.gov.au/view/whole/html/inforce/current/act-2007-064

- Workplace Surveillance Act 2005 No 47  
  https://legislation.nsw.gov.au/view/whole/html/inforce/current/act-2005-047

- Privacy and Personal Information Protection Act 1998 No 133  
  https://legislation.nsw.gov.au/view/whole/html/inforce/current/act-1998-133

- Health Records and Information Privacy Act 2002 No 71  
  https://legislation.nsw.gov.au/view/whole/html/inforce/current/act-2002-071

- Children (Education and Care Services) National Law (NSW), including Part 13 privacy provisions  
  https://legislation.nsw.gov.au/view/whole/html/inforce/current/act-2010-104a

## Commonwealth / Australia

- Office of the Australian Information Commissioner — Australian Privacy Principles  
  https://www.oaic.gov.au/privacy/australian-privacy-principles

- ACMA — Low Interference Potential Devices (LIPD) class licence  
  https://www.acma.gov.au/licences/low-interference-potential-devices-lipd-class-licence

- Federal Register — Radiocommunications (Low Interference Potential Devices) Class Licence 2025  
  https://www.legislation.gov.au/F2025L01047/latest/text
