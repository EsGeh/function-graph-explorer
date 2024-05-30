#include "fge/shared/parameter_utils.h"
#include <expected>
#include <QStringList>

namespace intern {

	// Types:
	using ParseResult = std::map<QString,std::variant<ParameterDescription,StateDescription>>;
	struct DummyError {};

	// Functions:
	ParseResult parseFunctionDataDescription(
			const QString& str
	);

	std::optional<DummyError> parseOptionalDouble(
			const uint index,
			const QStringList& words,
			double& d
	);

	std::map<QString,ParameterDescription> onlyParameterDescriptions(
			const ParseResult& dataDescription
	);

	std::map<QString,StateDescription> onlyStateDescriptions(
			const ParseResult& dataDescription
	);

} // namespace intern

FunctionDataDescription parseFunctionDataDescription(
		const QString& str
)
{
	const auto parseResult = intern::parseFunctionDataDescription(
			str
	);
	return FunctionDataDescription{
		.parameterDescriptions = intern::onlyParameterDescriptions( parseResult ),
		.stateDescriptions = intern::onlyStateDescriptions( parseResult )
	};
}

QString functionDataDescriptionToString( const FunctionDataDescription& dataDescription )
{
	QString str = "";
	for( auto [name, value] : dataDescription.parameterDescriptions ) {
		str += QString("parameter %2 %1 %3 %4 %5 %6\n")
			.arg( name )
			.arg( 1 ) // size
			.arg( value.initial )
			.arg( value.min )
			.arg( value.max )
			.arg( value.step )
		;
	}
	for( auto [name, value] : dataDescription.stateDescriptions ) {
		str += QString("state %2 %1\n")
			.arg( name )
			.arg( value.size )
		;
	}
	return str;
}

namespace intern {

ParseResult parseFunctionDataDescription(
		const QString& str
)
{
	decltype(intern::parseFunctionDataDescription(str)) ret;
	auto qstringlist = str.split(
			"\n",
			Qt::SkipEmptyParts
	);
	for( auto lineRaw : qstringlist ) {
		auto line = lineRaw.trimmed();
		auto words = line.split(' ');
		if( words.size() < 1 ) {
			continue;
		}
		auto argOrState = words.at(0);
		// <param|argument> <size> <name> <initial> <min> <max> <step>
		if( argOrState == "parameter" || argOrState == "param" ) {
			if( words.size() < 3 ) {
				continue;
			}
			ParameterDescription param;
			auto parseError = parseOptionalDouble(3,words,param.initial);
			if( parseError ) continue;
			parseError = parseOptionalDouble(4,words,param.min);
			if( parseError ) continue;
			parseError = parseOptionalDouble(5,words,param.max);
			if( parseError ) continue;
			parseError = parseOptionalDouble(6,words,param.step);
			if( parseError ) continue;
			ret.insert({ words.at(2), param });
		}
		else if( argOrState == "state" ) {
			if( words.size() < 3 ) {
				continue;
			}
			bool ok = true;
			auto size = words.at(1).toInt( &ok, 10);
			if( !ok || size < 0 ) {
				continue;
			}
			ret.insert({ words.at(2), StateDescription{ .size = (uint )size } });
		}
		else {
			continue;
		}
	}
	return ret;
}

std::optional<DummyError> parseOptionalDouble(
		const uint index,
		const QStringList& words,
		double& d
)
{
	if( words.size() >= index+1 ) {
		bool ok;
		double val = words.at(index).toDouble( &ok );
		if( ok ) {
			d = val;
		}
		else {
			return DummyError{};
		}
	}
	return {};
}

std::map<QString,ParameterDescription> onlyParameterDescriptions(
		const ParseResult& dataDescription
)
{
	decltype(onlyParameterDescriptions(dataDescription)) ret;
	{
		for( auto& entry : dataDescription ) {
			if( auto value = std::get_if<ParameterDescription>( &entry.second ) ) {
				ret.insert({ entry.first, *value });
			}
		}
	}
	return ret;
}

std::map<QString,StateDescription> onlyStateDescriptions(
		const ParseResult& dataDescription
)
{
	decltype(onlyStateDescriptions(dataDescription)) ret;
	for( auto [name,eitherVal] : dataDescription ) {
		if( auto value = std::get_if<StateDescription>( &eitherVal ) ) {
			ret.insert({ name, *value });
		}
	}
	return ret;
}

}
