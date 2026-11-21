// DOWNWELL_OVERFLOW_VERIFY code of DownwellAmiga.cpp, moved verbatim (WR.2). Each part
// is included at its original place inside that verifier's
// conditional; DOWNWELL_VERIFY_PART selects it. No include guard.
#if DOWNWELL_VERIFY_PART == 27
            {
                std::uint64_t actual[5];amigaOverflowWitness(port,actual);
                const unsigned frame=port.tick();
                bool matched=true;
                for(unsigned group=0;group<5;++group){
                    if(frame<1||frame>1800||unsigned(actual[group]>>32)!=overflowWitness[frame-1][group*2]||
                       unsigned(actual[group])!=overflowWitness[frame-1][group*2+1]){
                        gDownwellOverflowFailureFrame=frame;gDownwellOverflowFailureGroup=group;
                        gDownwellOverflowActual[0]=unsigned(actual[group]>>32);
                        gDownwellOverflowActual[1]=unsigned(actual[group]);
                        downwellOverflowMismatch();matched=false;exitCode=downwellGameplayFailed(__LINE__);break;
                    }
                }
                if(!matched)break;
                ++gDownwellOverflowChecks;
                if((gDownwellOverflowChecks&127u)==0)downwellOverflowProgress();
            }
#endif
