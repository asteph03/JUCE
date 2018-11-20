#include "ARASampleProjectDocumentController.h"
#include "ARASampleProjectPlaybackRenderer.h"

ARASampleProjectPlaybackRenderer::ARASampleProjectPlaybackRenderer (ARADocumentController* documentController)
: ARAPlaybackRenderer (documentController)
{}

int ARASampleProjectPlaybackRenderer::getReadAheadSize() const
{
    int readAheadSizeBySampleRate = (int) (2.0 * getSampleRate() + 0.5);
    int readAheadSizeByBlockSize = 8 * getMaxSamplesPerBlock();
    return jmax (readAheadSizeBySampleRate, readAheadSizeByBlockSize);
}

std::unique_ptr<BufferingAudioReader> ARASampleProjectPlaybackRenderer::createBufferingAudioSourceReader (ARAAudioSource* audioSource)
{
    auto documentController = static_cast<ARASampleProjectDocumentController*> (audioSource->getDocument()->getDocumentController());
    auto newSourceReader = documentController->createBufferingAudioSourceReader (audioSource, getReadAheadSize());
    newSourceReader->setReadTimeout (2000); // TODO JUCE_ARA I set at a high value arbitrarily, but we should pick a better volume
    return std::unique_ptr<BufferingAudioReader> (newSourceReader);
}

void ARASampleProjectPlaybackRenderer::prepareToPlay (double sampleRate, int numChannels, int maxSamplesPerBlock)
{
    auto oldSampleRate = getSampleRate();
    auto oldNumChannels = getNumChannels();
    auto oldMaxSamplesPerBlock = getMaxSamplesPerBlock();
    auto oldReadAheadSize = getReadAheadSize();

    ARAPlaybackRenderer::prepareToPlay(sampleRate, numChannels, maxSamplesPerBlock);

    if ((oldSampleRate != getSampleRate()) ||
        (oldNumChannels != getNumChannels()) ||
        (oldMaxSamplesPerBlock != getMaxSamplesPerBlock()) ||
        (oldReadAheadSize != getReadAheadSize()))
    {
        for (auto& readerPair : audioSourceReaders)
            readerPair.second = createBufferingAudioSourceReader (readerPair.first);
    }
}

// this function renders playback regions in the ARA document that have been
// a) added to this playback renderer instance and
// b) lie within the time range of samples being renderered (in project time)
// effectively making this plug-in a pass-through renderer
void ARASampleProjectPlaybackRenderer::processBlock (AudioBuffer<float>& buffer, int64 timeInSamples, bool isPlayingBack)
{
    jassert (buffer.getNumSamples() <= getMaxSamplesPerBlock());

    // zero out samples first, then add region output on top
    for (int c = 0; c < buffer.getNumChannels(); c++)
        FloatVectorOperations::clear (buffer.getArrayOfWritePointers()[c], buffer.getNumSamples());

    if (! isPlayingBack)
        return;

    // render back playback regions that lie within this range using our buffered ARA samples
    using namespace ARA;
    ARASamplePosition sampleStart = timeInSamples;
    ARASamplePosition sampleEnd = timeInSamples + buffer.getNumSamples();
    for (PlugIn::PlaybackRegion* playbackRegion : getPlaybackRegions())
    {
        // get the audio source for this region and make sure we have an audio source reader for it
        auto audioSource = static_cast<ARAAudioSource*> (playbackRegion->getAudioModification()->getAudioSource());
        if (audioSourceReaders.count (audioSource) == 0)
            continue;

        // render silence if access is currently disabled
        if (! audioSource->isSampleAccessEnabled())
            continue;

        // this simplified test code "rendering" only produces audio if sample rate and channel count match
        if ((audioSource->getChannelCount() != getNumChannels()) || (audioSource->getSampleRate() != getSampleRate()))
            continue;

        // evaluate region borders in song time, calculate sample range to copy in song time
        ARASamplePosition regionStartSample = playbackRegion->getStartInPlaybackSamples (getSampleRate());
        if (sampleEnd <= regionStartSample)
            continue;

        ARASamplePosition regionEndSample = playbackRegion->getEndInPlaybackSamples (getSampleRate());
        if (regionEndSample <= sampleStart)
            continue;

        ARASamplePosition startSongSample = jmax (regionStartSample, sampleStart);
        ARASamplePosition endSongSample = jmin (regionEndSample, sampleEnd);

        // calculate offset between song and audio source samples, clip at region borders in audio source samples
        // (if a plug-in supports time stretching, it will also need to reflect the stretch factor here)
        ARASamplePosition offsetToPlaybackRegion = playbackRegion->getStartInAudioModificationSamples() - regionStartSample;

        // clamp sample ranges within the range we're rendering
        ARASamplePosition startAvailableSourceSamples = jmax<ARASamplePosition> (0, playbackRegion->getStartInAudioModificationSamples());
        ARASamplePosition endAvailableSourceSamples = jmin (audioSource->getSampleCount(), playbackRegion->getEndInAudioModificationSamples());

        startSongSample = jmax (startSongSample, startAvailableSourceSamples - offsetToPlaybackRegion);
        endSongSample = jmin (endSongSample, endAvailableSourceSamples - offsetToPlaybackRegion);

        // use the buffered audio source reader to read samples into the audio block        
        // TODO JUCE_ARA bug here:
        // If regions overlap, the later region will overwrite the output of the earlier one!
        // (this is visible in our region sequence view, but not audible in most hosts since
        // they typically use a single region per renderer...)
        // Only the first region that is actually read may read directly into the buffer,
        // all later regions must read to a temporary buffer and add from there to the output.
        // prepareToPlay must create that buffer if there's more than one region.
        int startInDestBuffer = (int) (startSongSample - sampleStart);
        int startInSource = (int) (startSongSample + offsetToPlaybackRegion);
        int numSamplesToRead = (int) (endSongSample - startSongSample);
        audioSourceReaders[audioSource]->
            readSamples ((int**) buffer.getArrayOfWritePointers (), buffer.getNumChannels (), startInDestBuffer, startInSource, numSamplesToRead);
    }
}

// every time we add a playback region, make sure we have a buffered audio source reader for it
// we'll use this reader to pull samples from our ARA host and render them back in the audio thread
void ARASampleProjectPlaybackRenderer::didAddPlaybackRegion (ARA::PlugIn::PlaybackRegion* playbackRegion) noexcept
{
    auto audioSource = static_cast<ARAAudioSource*> (playbackRegion->getAudioModification()->getAudioSource());
    if (audioSourceReaders.count (audioSource) == 0)
        audioSourceReaders.emplace (audioSource, createBufferingAudioSourceReader (audioSource));
}

// we can delete the reader associated with this playback region's audio source
// if no other playback regions in the playback renderer share the same audio source
void ARASampleProjectPlaybackRenderer::willRemovePlaybackRegion (ARA::PlugIn::PlaybackRegion* playbackRegion) noexcept
{
    auto audioSource = playbackRegion->getAudioModification()->getAudioSource();
    for (auto otherPlaybackRegion : getPlaybackRegions())
        if (playbackRegion != otherPlaybackRegion)
            if (otherPlaybackRegion->getAudioModification()->getAudioSource() == audioSource)
                return;

    audioSourceReaders.erase (static_cast<ARAAudioSource*> (audioSource));
}
