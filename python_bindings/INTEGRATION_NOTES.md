# Python API integration: phases 3 and 4

Target: `integrate/upstream-python-bindings` in the native Windows checkout.

| Source commit | Integration result |
| --- | --- |
| `4c7e8e703` | Type hints, documentation, and signature tests were already present, extended by phase one to accept ISO strings in `run()`. Retained that newer behavior. |
| `d6bf49be6` | Added `get_object_names_by_class()` and retained `get_objects_by_class()` as its compatibility alias; included the alias regression. |
| `bee404954` | Applied README API corrections while preserving native Windows build instructions. |
| `66548cc9d` | Existing platform-specific library extensions and portable build preparation retained. Added missing delta CSV fixtures, runtime-root precedence, fixture checks, and Linux/macOS wheel workflow support. |
| `86f3375e0` | Upstream parent `5aef1a5fe` is already an ancestor of this branch. Its Python-binding delta is formatting-only. Did not replay the broad historical engine merge over the repaired current engine. |
| `32e8ead91` | Integrated trusted-publishing and artifact changes; aligned build targets with current package configuration instead of its historical Python/version list. |
| `c8e962764`, `53665c80b` | Historical version bumps to 1.0.13 and 1.0.12 are superseded by existing 1.0.18a1; no downgrade. |
| `f9748d677` | Added quoted-timestamp normalization to both Python conversion helpers. |

Additional fixes found during validation:

- Windows subprocess test paths are represented safely as Python literals.
- Temporary-directory tests stop the worker before removing its working directory.
- Property fallback regressions use a substation fixture, not a meter with different published properties.
- Invalid install roots leave runtime environment variables unchanged; environment tests restore their state.
- Native extension version metadata preserves the prerelease suffix.
- Python minimum is 3.10, matching runtime union annotations used by the API.
- Wheel CI builds the native core and all modules before packaging; macOS uses portable CPU counts and Linux-only repair steps stay Linux-only.
- Removed a job-level matrix condition: GitHub evaluates job conditions before expanding the matrix ([workflow syntax](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax)).
- Updated cibuildwheel to 3.3.0 for the existing Python 3.14 build target ([changelog](https://cibuildwheel.pypa.io/en/stable/changelog/)).
- Added Windows x64 wheel builds to the cross-platform cibuildwheel workflow.
- Wheel smoke tests now create an isolated worker and read its clock, covering native DLL loading and worker startup in addition to importing the extension.
- Bundled runtime paths take precedence over source-tree module paths, preventing `GRIDLABD_HOME` or `GRIDLABD_ROOT` overrides from mixing incompatible DLLs during worker initialization.
- Removed unconditional installation-validation output from stderr so non-verbose message capture remains silent.
- Removed the duplicate legacy TestPyPI workflow; `build-python-wheels.yml` is now the single cross-platform build and publishing workflow.
- Configured trusted publishing so pushes created by merges to `develop` and `feature/1478` publish to TestPyPI, while pushes to `main` publish to production PyPI.

Validation is on Windows x64 with MSVC Release and Python 3.12. Linux/macOS workflows are statically checked but have not been executed from this task. No packages have been published.

Existing skipped tests cover `--threadcount` (known hang) and `--check` (internal validation failure); their skips are unchanged.

Validation results: editable install and standalone Windows wheel each passed 171 tests with 2 existing skips. The wheel was installed in a separate virtual environment and tested from outside the repository. Both installations report `1.0.18a1`. Reports are in `out/python-api-phase34-results.xml` and `out/python-api-wheel-results.xml`; the wheel is in `out/wheels/`.
