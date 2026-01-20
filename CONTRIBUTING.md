<!--
Copyright Threaded Pixel Factory. All Rights Reserved.
Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.
-->

# Contributing to UETPFCore

Thank you for your interest in contributing to UETPFCore! This document provides guidelines for contributing to the project.

## Code of Conduct

### Our Pledge
We are committed to providing a welcoming and inspiring community for all. We are all learning and it is all a learning process. Please be respectful and constructive in all interactions. Great work refines all of us and kindness leads to great work.

### Our Standards
- Be respectful and inclusive
- Accept constructive criticism gracefully
- Focus on what's best for the community
- Show empathy towards other contributors

## How to Contribute

### Reporting Bugs

**Before submitting:**
1. Check existing GitHub Issues to avoid duplicates
2. Verify the bug in the latest version
3. Collect reproduction steps and logs

**Bug Report Template:**
```
**Description:**
Clear description of the bug

**Steps to Reproduce:**
1. Step one
2. Step two
3. ...

**Expected Behavior:**
What should happen

**Actual Behavior:**
What actually happens

**Environment:**
- OS: Windows 10/11
- UE Version: 5.7.x
- Commit/Version: abc123

**Logs:**
Paste relevant log output from Saved/Logs/

**Screenshots:**
If applicable
```

### Suggesting Features

**Feature Request Template:**
```
**Problem:**
What problem does this solve?

**Proposed Solution:**
Your suggested implementation

**Alternatives Considered:**
Other approaches you've thought about

**Use Case:**
Real-world scenario where this is needed

**Impact:**
Who benefits and how?
```

### Submitting Pull Requests

#### Before You Start
1. **Discuss first**: Open an issue to discuss major changes
2. **Check existing PRs**: Avoid duplicate work
3. **Fork the repo**: Work in your own fork

#### Development Workflow

1. **Create a branch:**
```bash
git checkout -b feat-your-feature-name
# or
git checkout -b fix-bug-description
```

2. **Make your changes:**
- Follow the coding standards (see below)
- Add comments for complex logic
- Update documentation if needed

3. **Test your changes:**
- Compile without errors
- Test in PIE (Play In Editor)
- Verify no regressions

4. **Commit your changes:**
```bash
git add .
git commit -m "feat: Add new subsystem for XYZ"
# or
git commit -m "fix: Resolve crash in EnvironmentSubsystem"
```

**Commit Message Format:**
```
<type>: <description>

[optional body]

[optional footer]
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, no logic change)
- `refactor`: Code refactoring
- `test`: Adding/updating tests
- `chore`: Build process, dependencies, etc.

**Examples:**
```
feat: Add temperature-based biome blending

Implements temperature gradient support in BiomeSubsystem,
allowing smooth transitions between hot and cold biomes.

Closes #123
```

```
fix: Prevent crash when querying unregistered surface

Added null check in SurfaceQuerySubsystem::QuerySurface()
to return default spec when surface ID not found.

Fixes #456
```

5. **Push to your fork:**
```bash
git push origin feature/your-feature-name
```

6. **Open a Pull Request:**
- Fill out the PR template
- Link related issues
- Provide clear description
- Add screenshots/videos if UI changes

#### Pull Request Template
```
**Description:**
Clear description of changes

**Motivation:**
Why is this change needed?

**Changes Made:**
- Change 1
- Change 2
- ...

**Testing:**
How was this tested?

**Checklist:**
- [ ] Code compiles without errors
- [ ] Code follows project style guidelines
- [ ] Documentation updated (if needed)
- [ ] No proprietary content included
- [ ] Tested in PIE
- [ ] No performance regressions

**Related Issues:**
Closes #123
Relates to #456
```

## Coding Standards

### General Principles
- **Clarity over cleverness**: Code should be readable
- **Comments**: Explain "why", not "what"
- **Modularity**: Keep functions small and focused
- **SOLID principles**: Follow where applicable

### C++ Style

#### Naming Conventions (UE Standard)
```cpp
// Classes: PascalCase with prefix
class UEnvironmentSubsystem;  // U for UObject
class AMyActor;                // A for AActor
class FMyStruct;               // F for struct
class EMyEnum;                 // E for enum

// Functions: PascalCase
void CalculateEnvironment();
float GetDensity() const;

// Variables: camelCase
float densityKgM3;
int32 numParticles;

// Member variables: Prefix with letter indicating scope
// (Unreal convention, (TODO:update with convention)
float Temperature;      // Public member (no prefix in this project atm)
FVector LocalPosition;  // Private (we follow clean style)

// Constants: PascalCase or ALL_CAPS
const float MaxTemperature = 1000.0f;
static constexpr float SPEED_OF_LIGHT = 299792458.0f;

// Booleans: Prefix with 'b'
bool bIsEnabled;
bool bHasAtmosphere;
```

#### File Organization
```cpp
// MyClass.h

// 1. Copyright header
// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0.

// 2. Include guard
#pragma once

// 3. Engine includes
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

// 4. Project includes
#include "SpecTypes.h"

// 5. Generated include (LAST)
#include "MyClass.generated.h"

// 6. Forward declarations (if needed)
class UMediumSpec;

// 7. Class declaration
/**
 * Detailed class documentation
 * Explains purpose, usage, and architecture
 */
UCLASS()
class UETPFCORE_API UMyClass : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Public interface

protected:
    // Protected members

private:
    // Private implementation
};
```

#### Comments and Documentation
```cpp
/**
 * Multi-line documentation comment for classes/functions
 * Explains purpose, parameters, return value, usage
 * 
 * @param Location World space location
 * @param bIncludeTemperature Whether to calculate temperature
 * @return Environment context at the location
 */
FEnvironmentContext GetEnvironmentAt(const FVector& Location, bool bIncludeTemperature = true);

// Single-line comment for implementation details
// Use these to explain "why" not "what"
float Density = 1.225f;  // Sea level air density
```

#### Code Formatting
```cpp
// Indentation: Tabs (UE standard)
// Braces: K&R style (UE standard)
void MyFunction()
{
    if (Condition)
    {
        // Code
    }
    else
    {
        // Code
    }
}

// Spacing
int32 x = 5;           // Space around operators
float y = x * 2.0f;    // Space around operators
MyFunction(a, b, c);   // Space after commas

// Line length: 120 characters max (soft limit)
```

### Blueprint Naming
```
BP_ActorName           // Blueprint actor
WBP_WidgetName         // Widget blueprint
DA_DataAssetName       // Data asset
M_MaterialName         // Material
MI_MaterialInstance    // Material instance
T_TextureName          // Texture
SM_StaticMeshName      // Static mesh
```

### Asset Organization
```
Content/
├── TPF/                    # Upcoming template content
│   ├── Examples/
│   ├── Materials/
│   └── Blueprints/
├── YourGame/               # Your game content
│   ├── Characters/
│   ├── Environments/
│   ├── Specs/
│   │   ├── Mediums/
│   │   └── Surfaces/
│   └── Maps/
```

## Documentation Guidelines

### Code Documentation
- Document all public APIs
- Do your best to Explain complex algorithms clearly
- Provide usage examples
- Keep docs up-to-date with code

### Markdown Documentation
- Use clear headings
- Include code examples
- Add diagrams where helpful
- Link to related docs

## Testing Requirements

### For Bug Fixes
- Verify the bug is fixed
- Ensure no regressions
- Test edge cases

### For New Features
- Test happy path
- Test error conditions
- Test with example content
- Performance test (if relevant)

### Before Submitting PR
- [ ] Compile clean (no errors, no warnings)
- [ ] Test in PIE
- [ ] Check log for errors
- [ ] Verify no proprietary content
- [ ] Run on clean project (if major change)

## License

By contributing, you agree that your contributions will be licensed under the Apache License, Version 2.0.

All contributions must:
- Not include proprietary content
- Not include third-party code without proper licensing
- Be your original work or properly attributed

## Questions?

- **GitHub Issues**: For bugs and features
- **GitHub Discussions**: For questions and general discussion
- **Documentation**: Check ARCHITECTURE.md and IMPLEMENTATIONGUIDE.md

## Recognition

Contributors will be acknowledged in:
- CONTRIBUTORS.md file
- Release notes
- Project credits

Thank you for contributing to UETPFCore! 🎉

---

## Reviewer Guidelines

### Code Review Checklist
- [ ] Code follows style guidelines
- [ ] Changes are well-documented
- [ ] No breaking changes (or documented if necessary)
- [ ] Tests pass (when written)
- [ ] Performance impact acceptable
- [ ] No security issues
- [ ] License compliance

### Review Process
1. Automated checks (CI/CD when available)
2. Manual code review
3. Request changes if needed
4. Approve when ready
5. Merge (squash or merge commit)

---

If it is feasible for you, financial contributions are also a way to contribute to development.

Monthly or one-time support helps us continue to develop, maintain and improve this framework:

- 💖 **[GitHub Sponsors](https://github.com/sponsors/ThreadedPixelFactory)** - Recurring support helps the platform grow.
- ☕ **[Ko-fi](https://ko-fi.com/threadedpixelfactory)** - One-time tips or additional monthly support
- 🎨 **[Patreon](https://www.patreon.com/ThreadedPixelFactory)** - Supporters get Early access to exclusive development blog posts that are open to feedback before they're published. Your contributions help the platform grow!
- ⭐ **Star this repo** - A Free and easy way to help others discover the project

**Thank you for contributing to UETPFCore!**
