#ifndef OPENMW_PROCESSORCONTAINER_HPP
#define OPENMW_PROCESSORCONTAINER_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorContainer : public BaseObjectProcessor
    {
    public:
        ProcessorContainer()
        {
            BPP_INIT(ID_CONTAINER)
        }

        virtual void Do(WorldPacket &packet, WorldEvent &event)
        {
            BaseObjectProcessor::Do(packet, event);

            LOG_APPEND(Log::LOG_VERBOSE, "- action: %i, containerSubAction: %i", (int) event.action, (int) event.containerSubAction);

            // If we've received a request for information, comply with it
            if (event.action == mwmp::BaseEvent::Action::Request)
                event.sendCellContainers(ptrCellStore);
            // Otherwise, edit containers based on the information received
            else
                event.editContainers(ptrCellStore);
        }

    };
}

#endif //OPENMW_PROCESSORCONTAINER_HPP
