#include <pybind11/pybind11.h>
#include <inviwo/core/common/inviwoapplication.h>
#include <inviwo/core/network/processornetwork.h>
#include <inviwo/core/datastructures/image/layerramprecision.h>
#include <modules/pydata/processors/imagesourcebuffer.h>

namespace py = pybind11;
using namespace inviwo;

template <typename T>
T* getProcessor(std::string identifer) {
    auto inviwoApp = InviwoApplication::getPtr();
    if (!inviwoApp)
        throw std::runtime_error("Cannot get reference to Inviwo application");

    auto network = inviwoApp->getProcessorNetwork();
    auto processor = network->getProcessorByIdentifier(identifer);
    if (!processor)
        throw std::runtime_error("Cannot find processor " + identifer);

    auto typedProcessor = dynamic_cast<T*>(processor);
    if (!typedProcessor)
        throw std::runtime_error(identifer + " is not of the correct type");

    return typedProcessor;
}

void set_image(std::string processorIdentifier, py::buffer b) {
    auto imageSource = getProcessor<ImageSourceBuffer>(processorIdentifier);

    py::buffer_info info = b.request();
    if (info.ndim != 2)
        throw std::runtime_error("Incompatible buffer dimension");

    std::shared_ptr<LayerRAM> layerRAM;
    if (info.format == py::format_descriptor<float>::format()) {
        layerRAM = createLayerRAM(size2_t(info.shape[1], info.shape[0]), LayerType::Color, DataFloat32::get());
    } else {
        throw std::runtime_error("Incompatible format: " + info.format);
    }

    auto layerData = layerRAM->getData();
    memcpy(layerData, info.ptr, info.itemsize*info.size);

    auto layer = std::make_shared<Layer>(layerRAM);
    auto image = std::make_shared<Image>(layer);
    imageSource->setData(image);
}

PYBIND11_PLUGIN(inviwo_pydatad) {
    py::module m("inviwo_pydatad");

    m.def("set_image", &set_image);

#ifdef VERSION_INFO
    m.attr("__version__") = py::str(VERSION_INFO);
#else
    m.attr("__version__") = py::str("dev");
#endif

    return m.ptr();
}