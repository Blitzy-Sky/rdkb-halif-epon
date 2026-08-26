# Changelog

All notable changes to `rdkb-halif-epon` are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this
repository adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Every release heading below names a tag that exists in this repository. **The date in a release
heading is the author date of the commit that tag marks, not the date the tag object was created.**
The two are not interchangeable here - for `v1.0.0` they are more than two months apart - so where
this file or the specification prints a date it says which of the two it is. A `git describe --tags`
string such as `v1.0.0-6-g<commit>` denotes a number of commits past a tag rather than a release, so
it never appears here as a release heading. Work that is not yet tagged is listed under
`Unreleased`.

Two version numbers travel with this repository, and they are not the same kind of thing:

- **Interface version** - the contract a caller compiles against. It is packed by
  `EPON_HAL_MAKE_VERSION` from `EPON_HAL_VERSION_MAJOR`, `EPON_HAL_VERSION_MINOR` and
  `EPON_HAL_VERSION_PATCH` in `epon_hal.h` (lines 41-45), and a caller reads it at runtime through
  `epon_hal_get_version()`. It is currently **1.0.0**.
- **Release tag** - the point in this repository's history that a consumer checks out, recorded as
  the release headings in this file. The current tag is **v1.0.0**.

The two agree at 1.0.0 today. They are advanced independently, so a future release may move one
without moving the other.

## Unreleased

Changes that are merged but not yet part of a tagged release. Their base is
[`v1.0.0`](https://github.com/rdkcentral/rdkb-halif-epon/releases/tag/v1.0.0); no comparison link is
given, because the only endpoint available for one today is a branch tip rather than a tag, and a
link whose target moves would not stay accurate. A comparison link is added here once the release
that carries these changes is tagged.

### Added

- `docs/pages/halSpec.md` - the EPON HAL interface specification, covering the interface's
  architecture, runtime execution requirements, non-functional requirements and complete API
  surface. This is the repository's first specification document.
- `docs/generate_docs.sh` - entry point that builds the specification into browsable HTML with the
  `rdkcentral/hal-doxygen` generator, pinned at tag `1.2.0`.
- `docs/.gitignore` - excludes `build` (the generator clone), `output` (the generated HTML) and
  `node_modules` from version control, so a documentation build leaves nothing to commit.
- Five symlinks in `docs/pages/`, so the repository-root governance and legal files render alongside
  the specification in the generated documentation: `CHANGELOG.md` to `../../CHANGELOG.md`,
  `CONTRIBUTING.md` to `../../CONTRIBUTING.md`, `COPYING.md` to `../../COPYING`, `LICENSE.md` to
  `../../LICENSE` and `NOTICE.md` to `../../NOTICE`. No root file was copied or altered to create
  them.
- `include/epon_hal.h` - a symlink to `../epon_hal.h`, which brings the header inside the
  documentation generator's input path so its inline documentation is extracted. The header itself
  does not move: `epon_hal.h` remains the published include path, and the symlink adds a second
  route to the same file rather than replacing the first.
- This changelog, which had no predecessor in this repository.

### Changed

- `README.md` is now a symlink to `docs/pages/halSpec.md`, so the interface specification is the
  repository's landing page. It previously carried a two-line description of the repository.
- The inline Doxygen documentation in `epon_hal.h` was expanded. Specifically:
  - All fifteen declared functions now carry a full caller contract: per-parameter direction,
    valid range, array length and buffer ownership; the pre-conditions the call imposes and the
    status returned when one is not met; the post-conditions on success and what is left
    unspecified on failure; what a caller should do about each documented status; and an explicit
    statement of whether the call may block and whether it is thread safe. Where the interface
    establishes neither - thread safety on all fifteen, and blocking on the fourteen other than
    `epon_hal_init`, whose three-minute `EPON_HAL_ERROR_TIMEOUT` bound does establish it - the
    block records the property as unspecified rather than asserting a behaviour.
  - Return documentation is now classified by what each function returns. `epon_hal_get_version`
    returns a value, so it describes the packed `0xMMmmpppp` encoding under `@return` and carries
    no `@retval`; the other fourteen return `epon_hal_return_t` and keep `@retval` per status. **No
    `@retval` was added, removed or reworded** - the set of status codes each function documents is
    identical to 1.0.0.
  - Nine types that carried no description gained one: `epon_onu_status_t`, `epon_llid_mode_t`,
    `epon_llid_state_t`, `epon_llid_forwarding_state_t`, `epon_llid_list_t`,
    `epon_onu_manufacturer_info_t`, `epon_encryption_mode_t`, `epon_interface_list_t` and
    `epon_hal_config_t`. The description on `epon_hal_link_info_t` was corrected: it read
    "EPON operational configuration", which describes neither of the two members it introduces.

### Fixed

- The `status_callback`, `alarm_callback` and `interface_status_callback` members of
  `epon_hal_config_t` used the *trailing* comment marker `/**<` in *leading* position, so Doxygen
  attached all three descriptions to the preceding `dpoe_supported` member and left the three
  callbacks undocumented in the generated output. They now use the leading `/**` form.
- Two comments identified a registered ONU by the name `OAM_REGISTERED`, which this interface does
  not declare. Both now name the declared enumerators `EPON_ONU_STATUS_REGISTRATION` and
  `EPON_ONU_INTF_STATUS_LINK_UP`.

**Every change to `epon_hal.h` recorded under `Changed` and `Fixed` above is comment-only.** No
declaration, signature, typedef, macro value, enumerator, struct member, preprocessor branch or
include was altered, so the API surface is unchanged, the interface version stays at 1.0.0, and a
vendor implementation built against 1.0.0 does not need re-qualification.

## 1.0.0 - 2026-01-23

First tagged release, at tag
[`v1.0.0`](https://github.com/rdkcentral/rdkb-halif-epon/releases/tag/v1.0.0). The heading date,
2026-01-23, is the author date of the commit that `v1.0.0` marks. The annotated tag object itself
was created later, on 2026-03-31. Both are given because the two are more than two months apart, so
a reader who needs one of them should not have to guess which one a bare date means.

It publishes an interface only - this repository ships the public header and no implementation,
build manifest or library.

### Added

- Public EPON HAL interface header `epon_hal.h`, declaring the interface for an EPON optical
  network unit (ONU) at packed API version 1.0.0 (`EPON_HAL_API_VERSION`). The initial draft
  declared fourteen functions
  ([`c6a44bc`](https://github.com/rdkcentral/rdkb-halif-epon/commit/c6a44bc1d06fd71f56c3e629d66677e16cf17b23),
  [#2](https://github.com/rdkcentral/rdkb-halif-epon/pull/2)).
- `epon_hal_deinit`, which completes the initialization lifecycle: it releases the resources
  `epon_hal_init` allocated, after which `epon_hal_init` must be called again before any other
  function is used. It takes the declared surface to fifteen functions
  ([`3be441a`](https://github.com/rdkcentral/rdkb-halif-epon/commit/3be441a60c6d05940d0626b9397528dd84579a26),
  [#4](https://github.com/rdkcentral/rdkb-halif-epon/pull/4)).
- The fifteen declared functions: `epon_hal_init`, `epon_hal_deinit`, `epon_hal_get_version`,
  `epon_hal_get_link_info`, `epon_hal_get_llid_info`, `epon_hal_get_link_stats`,
  `epon_hal_get_transceiver_stats`, `epon_hal_get_olt_info`, `epon_hal_get_manufacturer_info`,
  `epon_hal_get_interface_list`, `epon_hal_clear_stats`, `epon_hal_reset_onu`,
  `epon_hal_factory_reset`, `epon_hal_set_oam_log_mask` and `dpoe_hal_get_cpe_mac_table`.
- Typed return contract `epon_hal_return_t`, returned by every function except
  `epon_hal_get_version`, together with the `EPON_HAL_ERROR_WRONG_PON_MODE` status for hardware
  configured for a non-EPON PON mode
  ([`3be441a`](https://github.com/rdkcentral/rdkb-halif-epon/commit/3be441a60c6d05940d0626b9397528dd84579a26),
  [#4](https://github.com/rdkcentral/rdkb-halif-epon/pull/4)).
- Interface data types for configuration and callback registration, optical link and transceiver
  statistics, LLID records, ONU manufacturer identity, OLT discovery data, active interface lists,
  DPoE CPE MAC tables, and both the standard IEEE 802.3ah alarms and the vendor-specific DPoE
  alarms.
- Generic HAL logging front end - the `hal_log_level_t` severity ladder, the `HAL_LOG` macro, and a
  `HAL_LOG_FUNCTION` backend an implementer may redefine before including the header.
- The Apache-2.0 `LICENSE` and its accompanying `NOTICE`, a `COPYING` symlink to `LICENSE`, and a
  `CONTRIBUTING.md` directing a contributor to fork the repository, send a pull request and sign the
  RDK Contributor License Agreement, for project compliance
  ([`7f26d88`](https://github.com/rdkcentral/rdkb-halif-epon/commit/7f26d8800917310eba9ba842c4baa16174244aec),
  [#3](https://github.com/rdkcentral/rdkb-halif-epon/pull/3)).
- Contributor License Agreement check as the `.github/workflows/cla.yml` GitHub Actions workflow
  ([`3b6f189`](https://github.com/rdkcentral/rdkb-halif-epon/commit/3b6f1899677d9f5eb7e6af98ddd6d078fff84aa8),
  [#1](https://github.com/rdkcentral/rdkb-halif-epon/pull/1)).
- Repository bootstrap with an initial `README.md`
  ([`c99a9c4`](https://github.com/rdkcentral/rdkb-halif-epon/commit/c99a9c45e7c73ba0f726b2c44a9bc88006c42282)).
