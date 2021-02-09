#include "VGG16_Predicting.h"

void VGG16_Predicting::read_tensor_from_json(const Json::Value& root)
{
	unsigned int count = root["data"].size() - 1;
	for (unsigned int i = 0; i < count; i++)
		this->pTensor->push_back(root["data"][i].asDouble());
}

void VGG16_Predicting::create_tensor_msg(const std::vector<double> &prediction, std::string &doc)
{
    Json::Value root, data;
    Json::StreamWriterBuilder writerBuilder;
    std::ostringstream stream;

	// set type
	root["type"] = "tensor";
	// set tensor
	for (int i = 0; i < pTensor->size(); i++)
		data[i] = prediction.at(i);
	// append to root
	root["data"] = data;

	// write
    std::unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());
    jsonWriter->write(root, &stream);	// write to stream 
	doc = stream.str();
}

VGG16_Predicting::VGG16_Predicting()
{
	pModule = nullptr;
	fname = "";
	pTensor = new std::vector<double>(TENSOR_SIZE);
	pPrediction = new std::vector<double>(N_CLASSES);
}

VGG16_Predicting::VGG16_Predicting(const std::string &full_path)
{
	pModule = nullptr;
	fname = full_path;
	pTensor = new std::vector<double>(TENSOR_SIZE);
	pPrediction = new std::vector<double>(N_CLASSES);
}

VGG16_Predicting::~VGG16_Predicting()
{
	if (nullptr != pModule) delete pModule;
	delete pTensor;
	delete pPrediction;
}

void VGG16_Predicting::load(void)
{
	if (pModule) delete pModule;
	if (fname.empty()) throw std::runtime_error("Module file is not specified!");
	pModule = new torch::jit::script::Module;
	*pModule = torch::jit::load(this->fname);
}

void VGG16_Predicting::load(const std::string& full_path)
{
	if (pModule) delete pModule;
	if (full_path.empty()) throw std::runtime_error("Module file is not specified!");
	fname = full_path;
    pModule = new torch::jit::script::Module;
	*pModule = torch::jit::load(full_path);
}

std::vector<double> VGG16_Predicting::predict(const torch::jit::IValue& tensor)
{
	std::vector<torch::jit::IValue> inputs;
    inputs.push_back(tensor);
	at::Tensor output = pModule->forward(inputs).toTuple().get()->elements().back().toTensor();
	std::vector<double> ret;
	ret.push_back(output.index({ 0, 0 }).item().toDouble());
	ret.push_back(output.index({ 0, 1 }).item().toDouble());
	ret.push_back(output.index({ 0, 2 }).item().toDouble());
	*pPrediction = ret;
	return ret;
}

std::vector<double> VGG16_Predicting::predict(void)
{
	torch::Tensor tensor = torch::tensor(*pTensor);
	std::vector<torch::jit::IValue> inputs;
    inputs.push_back(tensor);
	at::Tensor output = pModule->forward(inputs).toTuple().get()->elements().back().toTensor();
	std::vector<double> ret;
	ret.push_back(output.index({ 0, 0 }).item().toDouble());
	ret.push_back(output.index({ 0, 1 }).item().toDouble());
	ret.push_back(output.index({ 0, 2 }).item().toDouble());
	*pPrediction = ret;
	return ret;
}

bool VGG16_Predicting::readStream(std::string &msg)
{
	std::string input;
	in(input);
	if (input.empty()) return false;

	Json::String errs;
	Json::Value root;
	Json::CharReaderBuilder readerBuilder;

	std::unique_ptr<Json::CharReader> const reader(readerBuilder.newCharReader());
	if (!reader->parse(input.c_str(), input.c_str() + input.length(), &root, &errs)
		|| !errs.empty())
		throw std::runtime_error("IPC JSON read error!");
	if ("tensor" != root["type"].asString())
	{
		msg = input;
		return false;
	}
	read_tensor_from_json(root);
	return true;
}

void VGG16_Predicting::sendPrediction(const std::vector<double> &tensor)
{
	std::string msg;
	create_tensor_msg(tensor, msg);
	out(msg);
}

void VGG16_Predicting::sendPrediction(void)
{
	std::string msg;
	create_tensor_msg(*pPrediction, msg);
	out(msg);
}