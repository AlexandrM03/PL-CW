#include "stdafx.h"
#include "MFST.h"

namespace MFST {
#pragma region CONSTRUCTORS
	MfstState::MfstState() {
		lenta_position = 0;
		nrule = -1;
		nrulechain = -1;
	}

	MfstState::MfstState(short pposition, MFSTSTSTACK pst, short pnrulechain) {
		lenta_position = pposition;
		st = pst;
		nrulechain = pnrulechain;
	}

	MfstState::MfstState(short pposition, MFSTSTSTACK pst, short pnrule, short pnrulechain) {
		lenta_position = pposition;
		st = pst;
		nrule = pnrule;
		nrulechain = pnrulechain;
	}

	Mfst::MfstDiagnosis::MfstDiagnosis() {
		lenta_position = -1;
		rc_step = SURPRISE;
		nrule = -1;
		nrule_chain = -1;
	}

	Mfst::MfstDiagnosis::MfstDiagnosis(short plenta_position, RC_STEP prc_step, short pnrule, short pnrule_chain) {
		lenta_position = plenta_position;
		rc_step = prc_step;
		nrule = pnrule;
		nrule_chain = pnrule_chain;
	}

	Mfst::Mfst() { lenta = 0; lenta_size = lenta_position = 0; }
	Mfst::Mfst(LT::LexTable& plex, GRB::Greibach pgreibach) {
		greibach = pgreibach;
		lex = plex;
		lenta = new short[lenta_size = lex.size];
		for (int k = 0; k < lenta_size; k++)
			lenta[k] = GRB::Rule::Chain::T(lex.table[k].lexema);
		lenta_position = 0;
		st.push(greibach.stbottomT);
		st.push(greibach.startN);
		nrulechain = -1;
	}

#pragma endregion
	Mfst::RC_STEP Mfst::step(std::ostream& stream_out) {
		RC_STEP rc = SURPRISE;
		if (lenta_position < lenta_size) {
			if (GRB::Rule::Chain::isN(st.top())) {
				GRB::Rule rule;
				if ((nrule = greibach.getRule(st.top(), rule)) >= 0) {
					GRB::Rule::Chain chain;
					if ((nrulechain = rule.getNextChain(lenta[lenta_position], chain, nrulechain + 1)) >= 0) {
						MFST_TRACE1
							savestate(stream_out); st.pop(); push_chain(chain); rc = NS_OK;
						MFST_TRACE2
					}
					else {
						MFST_TRACE4("NS_NRCHAIN/NS_NR")
							saveddiagnosis(NS_NORULECHAIN); rc = resetstate(stream_out) ? NS_NORULECHAIN : NS_NORULE;
					}
				}
				else rc = NS_ERROR;
			}
			else if (st.top() == lenta[lenta_position]) {
				lenta_position++; st.pop(); nrulechain = -1; rc = TS_OK;
				MFST_TRACE3
			}
			else {
				MFST_TRACE4(TS_NOK / NS_NORULECHAIN) rc = resetstate(stream_out) ? TS_NOK : NS_NORULECHAIN;
			}
		}
		else {
			rc = LENTA_END;
			MFST_TRACE4(LENTA_END);
		}

		return rc;
	}

	bool Mfst::push_chain(GRB::Rule::Chain chain) {
		for (int k = chain.size - 1; k >= 0; k--)
			st.push(chain.nt[k]);
		return true;
	}

	bool Mfst::savestate(std::ostream& stream_out) {
		storestate.push(MfstState(lenta_position, st, nrule, nrulechain));
		MFST_TRACE6("SAVESTATE:", storestate.size());
		return true;
	}

	bool Mfst::resetstate(std::ostream& stream_out) {
		bool rc = false;
		MfstState state;
		if (rc = (storestate.size() > 0)) {
			state = storestate.top();
			lenta_position = state.lenta_position;
			st = state.st;
			nrule = state.nrule;
			nrulechain = state.nrulechain;
			storestate.pop();
			MFST_TRACE5("RESSTATE")
				MFST_TRACE2
		}

		return rc;
	}

	bool Mfst::saveddiagnosis(RC_STEP prc_step) {
		bool rc = false;
		short k = 0;

		while (k < MFST_DIAGN_NUMBER && lenta_position <= diagnosis[k].lenta_position)
			k++;

		if (rc = (k < MFST_DIAGN_NUMBER)) {
			diagnosis[k] = MfstDiagnosis(lenta_position, prc_step, nrule, nrulechain);

			for (int i = k + 1; i < MFST_DIAGN_NUMBER; i++)
				diagnosis[i].lenta_position = -1;
		}

		return rc;
	}

	bool Mfst::start(std::ostream& stream_out) {
		MFST_TRACE_START
		bool rc = false;
		RC_STEP rc_step = SURPRISE;
		char buf[MFST_DIAGN_MAXSIZE]{};
		rc_step = step(stream_out);
		while (rc_step == NS_OK || rc_step == NS_NORULECHAIN || rc_step == TS_OK || rc_step == TS_NOK)
			rc_step = step(stream_out);

		switch (rc_step) {
		case LENTA_END:
			MFST_TRACE4("------>LENTA_END")
				stream_out << "------------------------------------------------------------------------------------------   ------" << std::endl;
			sprintf_s(buf, MFST_DIAGN_MAXSIZE, "%d: ����� ����� %d, �������������� ������ �������� ��� ������", 0, lex.table[lex.size - 1].line);
			stream_out << std::setw(4) << std::left << 0 << "����� ����� " << lex.table[lex.size - 1].line << ", �������������� ������ �������� ��� ������" << std::endl;
			rc = true;
			break;
		case NS_NORULE:
			MFST_TRACE4("------>NS_NORULE")
				stream_out << "------------------------------------------------------------------------------------------   ------" << std::endl;
			stream_out << getDiagnosis(0, buf) << std::endl;
			stream_out << getDiagnosis(1, buf) << std::endl;
			stream_out << getDiagnosis(2, buf) << std::endl;
			break;
		case NS_NORULECHAIN:
			MFST_TRACE4("------>NS_NORULECHAIN") break;
		case NS_ERROR:
			MFST_TRACE4("------>NS_ERROR") break;
		case SURPRISE:
			MFST_TRACE4("------>NS_SURPRISE") break;
		}

		return rc;
	}

	char* Mfst::getCSt(char* buf) {
		short p;
		for (int k = (signed)st.size() - 1; k >= 0; --k) {
			p = st.c[k];
			buf[st.size() - 1 - k] = GRB::Rule::Chain::alphabet_to_char(p);
		}
		buf[st.size()] = '\0';
		return buf;
	}

	char* Mfst::getCLenta(char* buf, short pos, short n) {
		short i = 0, k = (pos + n < lenta_size) ? pos + n : lenta_size;

		for (int i = pos; i < k; i++)
			buf[i - pos] = GRB::Rule::Chain::alphabet_to_char(lenta[i]);

		buf[i - pos] = '\0';
		return buf;
	}

	char* Mfst::getDiagnosis(short n, char* buf) {
		char* rc = new char[200]{};
		int errid = 0;
		int lpos = -1;
		if (n < MFST_DIAGN_NUMBER && (lpos = diagnosis[n].lenta_position) >= 0) {
			errid = greibach.getRule(diagnosis[n].nrule).iderror;
			Error::ERROR err = Error::geterror(errid);
			sprintf_s(buf, MFST_DIAGN_MAXSIZE, "%d: ������ %d,	%s", err.id, lex.table[lpos].line, err.message);
			rc = buf;
		}

		return rc;
	}

	void Mfst::printrules(std::ostream& stream_out) {
		MfstState state;
		GRB::Rule rule;
		for (unsigned short i = 0; i < storestate.size(); i++) {
			state = storestate.c[i];
			rule = greibach.getRule(state.nrule);
			MFST_TRACE7
		}
	}

	bool Mfst::savededucation() {
		MfstState state;
		GRB::Rule rule;
		deducation.nrules = new short[deducation.size = storestate.size()];
		deducation.nrulechains = new short[deducation.size];

		for (unsigned short i = 0; i < storestate.size(); i++) {
			state = storestate.c[i];
			deducation.nrules[i] = state.nrule;
			deducation.nrulechains[i] = state.nrulechain;
		}

		return true;
	}

	void check_syntax(Lex::LEX lex, Log::LOG log, std::ostream& stream_out) {
		MFST::Mfst mfst(lex.lextable, GRB::getGreibach());
		if (!mfst.start(stream_out)) {
			std::cout << "Syntax errors. Check log file to get more info";
			exit(-1);
		}
		mfst.savededucation();
		mfst.printrules(stream_out);
		FST_TRACE_n = -1;
	}
}