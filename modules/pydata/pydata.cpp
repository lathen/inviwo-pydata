#include <pybind11/pybind11.h>
#include <inviwo/core/common/inviwoapplication.h>
#include <inviwo/core/network/processornetwork.h>
#include <inviwo/core/datastructures/image/layerramprecision.h>
#include <inviwo/core/datastructures/volume/volumeramprecision.h>
#include <modules/pydata/processors/imagesourcebuffer.h>
#include <modules/pydata/processors/volumesourcebuffer.h>

namespace py = pybind11;
using namespace inviwo;

// Return a processor from the network in the Inviwo application
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

// Return the numeric type corresponding to the Python buffer protocol format
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

// Return the strides for a packed data array stored in row-major order
std::vector<size_t> getStrides(const std::vector<size_t>& shape, size_t itemsize) {
    std::vector<size_t> strides(shape.size());
    size_t i = shape.size() - 1;
    strides[i] = itemsize;
    while (true) {
        if (i == 0)
            break;
        i--;
        strides[i] = strides[i + 1] * shape[i + 1];
    }
    return strides;
}

void set_image(std::string processorIdentifier, py::buffer b) {
    py::buffer_info info = b.request();
    auto imageSource = getProcessor<ImageSourceBuffer>(processorIdentifier);

    // Check the strides
    auto strides = getStrides(info.shape, info.itemsize);
    if (info.strides != strides)
        throw std::runtime_error("Data stride not supported (expects a packed row-major stored buffer)");

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
    // Note that the buffer shape is described in matrix notation (rows,columns)
    // but the layer uses an image notation (width,height) where width = columns and height = rows
    // The OpenGL coordinate frame (origin in the lower left corner) furthermore renders the
    // array up-side down, which I'm not yet sure how to handle in a stringent manner
    auto layerRAM = createLayerRAM(size2_t(info.shape[1], info.shape[0]), LayerType::Color, dataFormat);
    if (!layerRAM)
        throw std::runtime_error("Cannot allocate layer buffer");

    auto layerData = layerRAM->getData();
    memcpy(layerData, info.ptr, info.itemsize*info.size);

    auto layer = std::make_shared<Layer>(layerRAM);
    auto image = std::make_shared<Image>(layer);
    imageSource->setData(image);
}

void set_volume(std::string processorIdentifier, py::buffer b) {
    py::buffer_info info = b.request();
    auto volumeSource = getProcessor<VolumeSourceBuffer>(processorIdentifier);

    // Check the strides
    auto strides = getStrides(info.shape, info.itemsize);
    if (info.strides != strides)
        throw std::runtime_error("Data stride not supported (expects a packed row-major stored buffer)");

    // Determine the numeric type
    NumericType type = getNumericType(info.format);

    // Determine the number of components
    size_t components;
    if (info.ndim < 3 || info.ndim > 4)
        throw std::runtime_error("Incompatible buffer dimensions (expected 3 or 4)");
    if (info.ndim == 3)
        components = 1;
    else
        components = info.shape[3];

    if (components > 4)
        throw std::runtime_error("Too many components (expected maximum 4)");

    // Get the resulting data format
    auto dataFormat = DataFormatBase::get(type, components, info.itemsize * 8);
    if (!dataFormat)
        throw std::runtime_error("Data format not supported");

    // Create the volume RAM representation
    // Note that the buffer shape is described in matrix notation (rows,columns,slices)
    // but the volume uses an image notation (width,height,slices) where width = columns and height = rows
    // The OpenGL coordinate frame (origin in the lower left corner) furthermore renders the
    // array up-side down, which I'm not yet sure how to handle in a stringent manner
    auto volumeRAM = createVolumeRAM(size3_t(info.shape[1], info.shape[0], info.shape[2]), dataFormat);
    if (!volumeRAM)
        throw std::runtime_error("Cannot allocate volume buffer");

    auto volumeData = volumeRAM->getData();
    memcpy(volumeData, info.ptr, info.itemsize*info.size);

    auto volume = std::make_shared<Volume>(volumeRAM);
    volumeSource->setData(volume);
}

PYBIND11_PLUGIN(inviwo_pydata) {
    py::module m("inviwo_pydata");

    m.def("set_image", &set_image);
    m.def("set_volume", &set_volume);

#ifdef VERSION_INFO
    m.attr("__version__") = py::str(VERSION_INFO);
#else
    m.attr("__version__") = py::str("dev");
#endif

    return m.ptr();
}
