// DOWNWELL_FIELD_VERIFY code of DownwellAmiga.cpp, moved verbatim (WR.2). Each part
// is included at its original place inside that verifier's
// conditional; DOWNWELL_VERIFY_PART selects it. No include guard.
#if DOWNWELL_VERIFY_PART == 15
    using FieldTape = downwell::GameplayFieldReentryVerification;
    alignas(FieldTape) static UBYTE fieldTapeStorage[sizeof(FieldTape)];
    auto& fieldTape = *new (fieldTapeStorage) FieldTape;
    const bool fieldPassed = fieldTape.run(port,collisionOwner,objects,collisionSprites,initialRingOrigin);
    gPreparedObjects=fieldTape.preparedResources.objectsChecked;
    gPreparedAlarms=fieldTape.preparedResources.alarmsChecked;
    gPreparedSprites=fieldTape.preparedResources.spritesChecked;
    gDownwellFieldChecks=fieldTape.checks;gDownwellFieldStage=fieldTape.stage;
    gDownwellFieldGameSteps=fieldTape.gameSteps;gDownwellFieldClearSteps=fieldTape.clearSteps;
    gDownwellFieldFirst=fieldTape.firstGeneration;gDownwellFieldLater=fieldTape.laterGeneration;
    gDownwellFieldRetry=fieldTape.retryGeneration;
    checkSceneStack(40);
    downwellFieldReentryReady();
    if(!fieldPassed)return 20;
#endif
