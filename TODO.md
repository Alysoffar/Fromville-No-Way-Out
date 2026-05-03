# Resident Evil Village Integration TODO

## Plan Steps:
- [x] Step 1: Delete assets/models/cottage_obj.mtl (already absent)
- [x] Step 2a: Rename globals in World.cpp (gCottage* → gVillage*)
- [x] Step 2b: Replace model loading in InitializeModels()
- [x] Step 2c: Update rendering in World::Render()
- [x] Step 2d: Set player spawn position in World::Initialize()
- [ ] Step 3: Verify assets exist (already confirmed)
- [ ] Step 4: Build project and fix errors
- [ ] Step 4: Run game and test village render/player spawn

## Next Action: Deleting cottage_obj.mtl
