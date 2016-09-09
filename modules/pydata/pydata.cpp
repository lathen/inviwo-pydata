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

NumericType getNumericType(std::string format) {
    
    if (format == py::format_descriptor<float>::format() ||
        format == py::format_descriptor<double>::format())
        return NumericType::Float;
    else if (format == py::format_descriptor<char>::format() ||
        format == py::format_descriptor<short>::format() ||
        format == py::format_descriptor<int>::format() ||
        format == py::format_descriptor<long int>::format())
        return NumericType::SignedInteger;
    else if (format == py::format_descriptor<unsigned char>::format() ||
        format == py::format_descriptor<unsigned short>::format() ||
        format == py::format_descriptor<unsigned int>::format() ||
        format == py::format_descriptor<unsigned long int>::format())
        return NumericType::UnsignedInteger;
    else
        throw std::runtime_error("Data type not supported");
}

void set_image(std::string processorIdentifier, py::buffer b) {
    auto imageSource = getProcessor<ImageSourceBuffer>(processorIdentifier);

    py::buffer_info info = b.request();

    // Determine the numeric type
    NumericType type = getNumericType(info.format);

    // Determine the number of components
    size_t components;
    if (info.ndim < 2 || info.ndim > 3)
        throw std::runtime_error("Incompatible buffer dimensions (expected 2 or 3)");
    if (info.ndim == 2)
        components = 1;
    else
        components = info.shape[2];

    if (components > 4)
        throw std::runtime_error("Too many components (expected maximum 4)");

    // Get the resulting data format
    auto dataFormat = DataFormatBase::get(type, components, info.itemsize * 8);
    if (!dataFormat)
        throw std::runtime_error("Data format not supported");

    // Create the layer RAM representation
    std::shared_ptr<LayerRAM> layerRAM;
    layerRAM = createLayerRAM(size2_t(info.shape[1], info.shape[0]), LayerType::Color, dataFormat);
    if (!layerRAM)
        throw std::runtime_error("Cannot allocate layer buffer");

    auto layerData = layerRAM->getData();
    memcpy(layerData, info.ptr, info.itemsize*info.size);

    auto layer = std::make_shared<Layer>(layerRAM);
    auto image = std::make_shared<Image>(layer);
    imageSource->setData(image);
}

PYBIND11_PLUGIN(inviwo_pydata) {
    py::module m("inviwo_pydata");

    m.def("set_image", &set_image);

#ifdef VERSION_INFO
    m.attr("__version__") = py::str(VERSION_INFO);
#else
    m.attr("__version__") = py::str("dev");
#endif

    return m.ptr();
}
